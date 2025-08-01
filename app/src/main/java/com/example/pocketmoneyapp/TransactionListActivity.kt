// PocketMoneyApp/app/src/main/java/com/example/pocketmoneyapp/TransactionListActivity.kt

package com.example.pocketmoneyapp

import android.app.Activity
import android.app.AlertDialog
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
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
    private lateinit var addTransactionButton: Button

    // 일관성을 위해 모든 JNI 함수는 companion object 밖으로 이동합니다.
    private external fun createTransactionNative(
        walletId: Int,
        description: String,
        amount: Long,
        type: Int,
        transactionDate: String
    ): Boolean

    private external fun getTransactionsByWalletNative(walletId: Int): Array<TransactionDto>

    // 새로 추가하거나 기존에 옮겨진 JNI 함수들
    private external fun updateTransactionNative(
        id: Int,
        walletId: Int, // walletId도 함께 전달
        description: String,
        amount: Long,
        type: Int,
        transactionDate: String
    ): Boolean

    private external fun deleteTransactionNative(id: Int, walletId: Int): Boolean // walletId도 함께 전달


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_transaction_list)

        // System.loadLibrary("native_core")는 MainActivity에서 이미 로드했으므로 여기서 다시 할 필요는 없습니다.
        // 하지만 혹시 TransactionListActivity가 단독 실행될 수도 있는 상황을 대비해 추가해 두는 것도 나쁘지 않습니다.
        // 여기서는 MainActivity가 시작점이라는 가정 하에 생략합니다.

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
        addTransactionButton = findViewById(R.id.addTransactionButton)

        walletNameTextView.text = walletName
        walletBalanceTextView.text = "잔액: ${String.format("%,d", walletBalance)}원"

        // TransactionAdapter에 클릭 리스너 추가
        transactionAdapter = TransactionAdapter(this, transactionList) { transaction ->
            // 트랜잭션 아이템 클릭 시 수정/삭제 다이얼로그 표시
            showEditDeleteTransactionDialog(transaction)
        }
        transactionRecyclerView.layoutManager = LinearLayoutManager(this)
        transactionRecyclerView.adapter = transactionAdapter

        addTransactionButton.setOnClickListener {
            showAddTransactionDialog()
        }

        loadTransactions()
    }

    override fun onResume() {
        super.onResume()
        loadTransactions()
        // 잔액은 MainActivity에서 갱신하므로 여기서는 갱신하지 않습니다.
    }

    override fun finish() {
        setResult(Activity.RESULT_OK, Intent().apply {
            putExtra("SHOULD_REFRESH_WALLETS", true)
        })
        super.finish()
    }

    private fun loadTransactions() {
        Log.d("TransactionListActivity", "Loading transactions for wallet ID: $walletId")
        // 인스턴스 메서드로 호출
        val transactionsArray = getTransactionsByWalletNative(walletId)
        transactionList.clear()
        transactionList.addAll(transactionsArray.toList())
        transactionAdapter.notifyDataSetChanged()
        Log.d("TransactionListActivity", "Loaded ${transactionList.size} transactions.")
    }

    private fun showAddTransactionDialog() {
        val dialogView = LayoutInflater.from(this).inflate(R.layout.dialog_add_transaction, null)
        val builder = AlertDialog.Builder(this)
            .setView(dialogView)
            .setTitle("새 트랜잭션 추가")

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

            if (description.isEmpty() || amountStr.isEmpty() || selectedTypeId == -1) {
                Toast.makeText(this, "모든 필드를 입력해주세요.", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            val amount = amountStr.toLongOrNull()
            if (amount == null || amount <= 0) {
                Toast.makeText(this, "유효한 금액을 입력해주세요.", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            val transactionType = when (selectedTypeId) {
                R.id.radioIncome -> TransactionType.INCOME
                R.id.radioExpense -> TransactionType.EXPENSE
                else -> TransactionType.EXPENSE
            }

            val currentDateTime = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(Date())

            // 인스턴스 메서드로 호출
            val success = createTransactionNative(
                walletId,
                description,
                amount,
                transactionType.value,
                currentDateTime
            )

            if (success) {
                Toast.makeText(this, "트랜잭션이 성공적으로 추가되었습니다.", Toast.LENGTH_SHORT).show()
                dialog.dismiss()
                loadTransactions()
            } else {
                Toast.makeText(this, "트랜잭션 추가 실패.", Toast.LENGTH_SHORT).show()
            }
        }

        addCancelButton.setOnClickListener {
            dialog.dismiss()
        }
    }

    // 새롭게 추가되는 함수: 트랜잭션 수정/삭제 다이얼로그
    private fun showEditDeleteTransactionDialog(transaction: TransactionDto) {
        val dialogView = LayoutInflater.from(this).inflate(R.layout.dialog_edit_delete_transaction, null)
        val builder = AlertDialog.Builder(this)
            .setView(dialogView)
            .setTitle("트랜잭션 수정/삭제")

        val dialog = builder.show()

        val descriptionEditText = dialogView.findViewById<EditText>(R.id.editTransactionDescriptionEditText)
        val amountEditText = dialogView.findViewById<EditText>(R.id.editTransactionAmountEditText)
        val typeRadioGroup = dialogView.findViewById<RadioGroup>(R.id.editTransactionTypeRadioGroup)
        val updateButton = dialogView.findViewById<Button>(R.id.updateTransactionConfirmButton)
        val deleteButton = dialogView.findViewById<Button>(R.id.deleteTransactionButton)
        val cancelButton = dialogView.findViewById<Button>(R.id.editTransactionCancelButton)

        // 현재 트랜잭션 정보로 UI 채우기
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

            if (description.isEmpty() || amountStr.isEmpty() || selectedTypeId == -1) {
                Toast.makeText(this, "모든 필드를 입력해주세요.", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            val amount = amountStr.toLongOrNull()
            if (amount == null || amount <= 0) {
                Toast.makeText(this, "유효한 금액을 입력해주세요.", Toast.LENGTH_SHORT).show()
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
                walletId, // walletId도 함께 전달
                description,
                amount,
                newTransactionType.value,
                currentDateTime
            )

            if (success) {
                Toast.makeText(this, "트랜잭션이 성공적으로 수정되었습니다.", Toast.LENGTH_SHORT).show()
                dialog.dismiss()
                loadTransactions() // 목록 갱신
            } else {
                Toast.makeText(this, "트랜잭션 수정 실패.", Toast.LENGTH_SHORT).show()
            }
        }

        deleteButton.setOnClickListener {
            AlertDialog.Builder(this)
                .setTitle("트랜잭션 삭제")
                .setMessage("정말로 이 트랜잭션을 삭제하시겠습니까?")
                .setPositiveButton("삭제") { _, _ ->
                    val success = deleteTransactionNative(transaction.id, walletId) // walletId도 함께 전달
                    if (success) {
                        Toast.makeText(this, "트랜잭션이 성공적으로 삭제되었습니다.", Toast.LENGTH_SHORT).show()
                        dialog.dismiss()
                        loadTransactions() // 목록 갱신
                    } else {
                        Toast.makeText(this, "트랜잭션 삭제 실패.", Toast.LENGTH_SHORT).show()
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