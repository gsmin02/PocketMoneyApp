package com.example.pocketmoneyapp

import android.app.Activity
import android.app.AlertDialog
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.RadioGroup
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.example.pocketmoneyapp.data.TransactionDto
import com.example.pocketmoneyapp.ui.TransactionAdapter
import com.example.pocketmoneyapp.data.TransactionType
import com.google.android.material.floatingactionbutton.FloatingActionButton // FAB import
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class TransactionListActivity : AppCompatActivity() {

    private var walletId: Int = -1
    private var walletName: String = ""
    private var walletBalance: Long = 0L

    private lateinit var walletNameTextView: TextView
    private lateinit var walletBalanceTextView: TextView
    private lateinit var transactionRecyclerView: RecyclerView
    private lateinit var transactionAdapter: TransactionAdapter
    private val transactionList = mutableListOf<TransactionDto>()
    private lateinit var noTransactionsTextView: TextView // Empty State TextView
    private lateinit var addTransactionFab: FloatingActionButton // FAB

    // 일관성을 위해 모든 JNI 함수는 companion object 밖으로 이동합니다.
    private external fun createTransactionNative(
        walletId: Int,
        description: String,
        amount: Long,
        type: Int,
        transactionDate: String
    ): Boolean

    private external fun getTransactionsByWalletNative(walletId: Int): Array<TransactionDto>

    private external fun updateTransactionNative(
        id: Int,
        walletId: Int,
        description: String,
        amount: Long,
        type: Int,
        transactionDate: String
    ): Boolean

    private external fun deleteTransactionNative(id: Int, walletId: Int): Boolean

    private external fun getWalletByIdNative(id: Int): com.example.pocketmoneyapp.data.WalletDto?


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_transaction_list)

        walletId = intent.getIntExtra("WALLET_ID", -1)
        walletName = intent.getStringExtra("WALLET_NAME") ?: "Unknown Wallet"
        walletBalance = intent.getLongExtra("WALLET_BALANCE", 0L)

        if (walletId == -1) {
            Log.e("TransactionListActivity", "Invalid Wallet ID received. Finishing activity.")
            finish()
            return
        }

        walletNameTextView = findViewById(R.id.walletNameTextView)
        walletBalanceTextView = findViewById(R.id.walletBalanceTextView)
        transactionRecyclerView = findViewById(R.id.transactionRecyclerView)
        noTransactionsTextView = findViewById(R.id.noTransactionsTextView) // Empty State TextView 초기화
        addTransactionFab = findViewById(R.id.addTransactionFab) // FAB 초기화

        walletNameTextView.text = walletName
        // 잔액은 MainActivity에서 로드 시 갱신하므로, 여기서는 초기 값만 사용
        // 실제로는 매번 loadTransactions() 시 MainActivity로 돌아가 잔액을 다시 로드하는 것이 더 정확할 수 있습니다.
        // 아니면, getWalletByIdNative를 여기서 다시 호출하여 최신 잔액을 가져오는 방법도 있습니다.
        walletBalanceTextView.text = "잔액: ${String.format("%,d", walletBalance)}원"

        transactionAdapter = TransactionAdapter(this, transactionList) { transaction ->
            showEditDeleteTransactionDialog(transaction)
        }
        transactionRecyclerView.layoutManager = LinearLayoutManager(this)
        transactionRecyclerView.adapter = transactionAdapter

        addTransactionFab.setOnClickListener { // FAB 클릭 리스너
            showAddTransactionDialog()
        }

        loadTransactions()
    }

    override fun onResume() {
        super.onResume()
        loadTransactions()
    }

    override fun finish() {
        // MainActivity로 돌아갈 때 지갑 목록 갱신 플래그 설정
        setResult(Activity.RESULT_OK, Intent().apply {
            putExtra("SHOULD_REFRESH_WALLETS", true)
        })
        super.finish()
    }

    private fun loadTransactions() {
        Log.d("TransactionListActivity", "Loading transactions for wallet ID: $walletId")

        // 1. 최신 지갑 정보 (잔액 포함)를 가져옵니다.
        val currentWallet = getWalletByIdNative(walletId)
        currentWallet?.let {
            // walletBalance = it.balance // 이제 이 변수는 필수가 아님
            walletBalanceTextView.text = "잔액: ${String.format("%,d", it.balance)}원" // UI 업데이트
            Log.d("TransactionListActivity", "Wallet balance updated to: ${it.balance}")
        } ?: run {
            Log.e("TransactionListActivity", "Failed to retrieve current wallet balance for ID: $walletId")
            // 지갑을 찾을 수 없는 경우에 대한 처리 (예: Activity 종료)
            Toast.makeText(this, "지갑 정보를 찾을 수 없습니다.", Toast.LENGTH_SHORT).show()
            finish()
            return
        }

        // 2. 트랜잭션 목록을 가져옵니다.
        val transactionsArray = getTransactionsByWalletNative(walletId)
        transactionList.clear()
        transactionList.addAll(transactionsArray.toList())
        transactionAdapter.notifyDataSetChanged()
        Log.d("TransactionListActivity", "Loaded ${transactionList.size} transactions.")

        // Empty State UI 가시성 관리
        if (transactionList.isEmpty()) {
            transactionRecyclerView.visibility = View.GONE
            noTransactionsTextView.visibility = View.VISIBLE
        } else {
            transactionRecyclerView.visibility = View.VISIBLE
            noTransactionsTextView.visibility = View.GONE
        }
    }

    private fun showAddTransactionDialog() {
        val dialogView = LayoutInflater.from(this).inflate(R.layout.dialog_add_transaction, null)
        val builder = AlertDialog.Builder(this)
            .setView(dialogView)
            .setTitle("새 거래내역 추가")

        val dialog = builder.show()

        val descriptionEditText = dialogView.findViewById<EditText>(R.id.transactionDescriptionEditText)
        val amountEditText = dialogView.findViewById<EditText>(R.id.transactionAmountEditText)
        val typeRadioGroup = dialogView.findViewById<RadioGroup>(R.id.transactionTypeRadioGroup)
        val addConfirmButton = dialogView.findViewById<Button>(R.id.addTransactionConfirmButton)
        val addCancelButton = dialogView.findViewById<Button>(R.id.addTransactionCancelButton)

        addConfirmButton.setOnClickListener {
            val description = descriptionEditText.text.toString().trim()
            val amountStr = amountEditText.text.toString().trim()
            val selectedTypeId = typeRadioGroup.checkedRadioButtonId

            if (description.isEmpty()) {
                descriptionEditText.error = "설명을 입력해주세요." // 입력 오류 피드백
                return@setOnClickListener
            }
            if (amountStr.isEmpty()) {
                amountEditText.error = "금액을 입력해주세요." // 입력 오류 피드백
                return@setOnClickListener
            }
            if (selectedTypeId == -1) {
                Toast.makeText(this, "유형을 선택해주세요.", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            val amount = amountStr.toLongOrNull()
            if (amount == null || amount <= 0) {
                amountEditText.error = "유효한 금액을 입력해주세요." // 입력 오류 피드백
                return@setOnClickListener
            }

            val transactionType = when (selectedTypeId) {
                R.id.radioIncome -> TransactionType.INCOME
                R.id.radioExpense -> TransactionType.EXPENSE
                else -> TransactionType.EXPENSE
            }

            val currentDateTime = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(Date())

            val success = createTransactionNative(
                walletId,
                description,
                amount,
                transactionType.value,
                currentDateTime
            )

            if (success) {
                Toast.makeText(this, "거래내역이 성공적으로 추가되었습니다.", Toast.LENGTH_SHORT).show()
                dialog.dismiss()
                loadTransactions()
            } else {
                Toast.makeText(this, "거래내역 추가 실패.", Toast.LENGTH_SHORT).show()
            }
        }

        addCancelButton.setOnClickListener {
            dialog.dismiss()
        }
    }

    private fun showEditDeleteTransactionDialog(transaction: TransactionDto) {
        val dialogView = LayoutInflater.from(this).inflate(R.layout.dialog_edit_delete_transaction, null)
        val builder = AlertDialog.Builder(this)
            .setView(dialogView)
            .setTitle("거래내역 수정/삭제")

        val dialog = builder.show()

        val descriptionEditText = dialogView.findViewById<EditText>(R.id.editTransactionDescriptionEditText)
        val amountEditText = dialogView.findViewById<EditText>(R.id.editTransactionAmountEditText)
        val typeRadioGroup = dialogView.findViewById<RadioGroup>(R.id.editTransactionTypeRadioGroup)
        val updateButton = dialogView.findViewById<Button>(R.id.updateTransactionConfirmButton)
        val deleteButton = dialogView.findViewById<Button>(R.id.deleteTransactionButton)
        val cancelButton = dialogView.findViewById<Button>(R.id.editTransactionCancelButton)

        descriptionEditText.setText(transaction.description)
        amountEditText.setText(transaction.amount.toString())
        when (transaction.type) {
            TransactionType.INCOME.value -> typeRadioGroup.check(R.id.editRadioIncome)
            TransactionType.EXPENSE.value -> typeRadioGroup.check(R.id.editRadioExpense)
        }

        updateButton.setOnClickListener {
            val description = descriptionEditText.text.toString().trim()
            val amountStr = amountEditText.text.toString().trim()
            val selectedTypeId = typeRadioGroup.checkedRadioButtonId

            if (description.isEmpty()) {
                descriptionEditText.error = "설명을 입력해주세요."
                return@setOnClickListener
            }
            if (amountStr.isEmpty()) {
                amountEditText.error = "금액을 입력해주세요."
                return@setOnClickListener
            }
            if (selectedTypeId == -1) {
                Toast.makeText(this, "유형을 선택해주세요.", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            val amount = amountStr.toLongOrNull()
            if (amount == null || amount <= 0) {
                amountEditText.error = "유효한 금액을 입력해주세요."
                return@setOnClickListener
            }

            val newTransactionType = when (selectedTypeId) {
                R.id.editRadioIncome -> TransactionType.INCOME
                R.id.editRadioExpense -> TransactionType.EXPENSE
                else -> TransactionType.EXPENSE
            }
            val currentDateTime = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(Date())

            val success = updateTransactionNative(
                transaction.id,
                walletId,
                description,
                amount,
                newTransactionType.value,
                currentDateTime
            )

            if (success) {
                Toast.makeText(this, "거래내역이 성공적으로 수정되었습니다.", Toast.LENGTH_SHORT).show()
                dialog.dismiss()
                loadTransactions()
            } else {
                Toast.makeText(this, "거래내역 수정 실패.", Toast.LENGTH_SHORT).show()
            }
        }

        deleteButton.setOnClickListener {
            AlertDialog.Builder(this)
                .setTitle("거래내역 삭제")
                .setMessage("정말로 이 거래내역을 삭제하시겠습니까?")
                .setPositiveButton("삭제") { _, _ ->
                    val success = deleteTransactionNative(transaction.id, walletId)
                    if (success) {
                        Toast.makeText(this, "거래내역이 성공적으로 삭제되었습니다.", Toast.LENGTH_SHORT).show()
                        dialog.dismiss()
                        loadTransactions()
                    } else {
                        Toast.makeText(this, "거래내역 삭제 실패.", Toast.LENGTH_SHORT).show()
                    }
                }
                .setNegativeButton("취소", null)
                .show()
        }

        cancelButton.setOnClickListener {
            dialog.dismiss()
        }
    }
}