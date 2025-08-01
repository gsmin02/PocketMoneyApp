package com.example.pocketmoneyapp

import android.app.Activity
import android.app.AlertDialog
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.example.pocketmoneyapp.data.WalletDto
import com.example.pocketmoneyapp.ui.WalletAdapter
import com.google.android.material.floatingactionbutton.FloatingActionButton // FAB import

class MainActivity : AppCompatActivity() {

    private lateinit var walletRecyclerView: RecyclerView
    private lateinit var walletAdapter: WalletAdapter
    private val walletList = mutableListOf<WalletDto>()
    private lateinit var noWalletsTextView: TextView // Empty State TextView
    private lateinit var addWalletFab: FloatingActionButton // FAB

    private external fun initializeNativeDb(dbPath: String)
    private external fun createWalletNative(name: String, description: String, balance: Long): Boolean
    private external fun getAllWalletsNative(): Array<WalletDto>
    private external fun updateWalletNative(id: Int, name: String, description: String, balance: Long): Boolean
    private external fun deleteWalletNative(id: Int): Boolean
    private external fun getWalletByIdNative(id: Int): WalletDto? // 변경된 부분: 널러블 반환 타입

    // 트랜잭션 목록 액티비티에서 돌아올 때 결과 처리를 위한 런처
    private val transactionListActivityResultLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode == Activity.RESULT_OK) {
            val data: Intent? = result.data
            val shouldRefreshWallets = data?.getBooleanExtra("SHOULD_REFRESH_WALLETS", false) ?: false
            if (shouldRefreshWallets) {
                Log.d("MainActivity", "Refreshing wallets after returning from TransactionListActivity.")
                loadWallets() // 지갑 목록 및 잔액 갱신
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Native 라이브러리 로드
        try {
            System.loadLibrary("native_core")
            Log.d("MainActivity", "Native library loaded.")
        } catch (e: UnsatisfiedLinkError) {
            Log.e("MainActivity", "Failed to load native library: ${e.message}")
            Toast.makeText(this, "Native 라이브러리 로드 실패: ${e.message}", Toast.LENGTH_LONG).show()
        }

        val dbPath = getDatabasePath("pocket_money.db").absolutePath
        initializeNativeDb(dbPath)
        Log.d("MainActivity", "Database initialized at: $dbPath")

        walletRecyclerView = findViewById(R.id.walletRecyclerView)
        noWalletsTextView = findViewById(R.id.noWalletsTextView) // Empty State TextView 초기화
        addWalletFab = findViewById(R.id.addWalletFab) // FAB 초기화

        // WalletAdapter에 클릭 리스너 추가
        walletAdapter = WalletAdapter(this, walletList,
            onWalletClick = { wallet ->
                // 지갑 아이템 클릭 시 트랜잭션 목록 화면으로 이동
                val intent = Intent(this, TransactionListActivity::class.java).apply {
                    putExtra("WALLET_ID", wallet.id)
                    putExtra("WALLET_NAME", wallet.name)
                    putExtra("WALLET_BALANCE", wallet.balance) // 최신 잔액 전달
                }
                transactionListActivityResultLauncher.launch(intent)
            },
            onWalletLongClick = { wallet ->
                // 지갑 아이템 길게 클릭 시 수정/삭제 다이얼로그 표시
                showEditDeleteWalletDialog(wallet)
                true // 이벤트 소비
            }
        )
        walletRecyclerView.layoutManager = LinearLayoutManager(this)
        walletRecyclerView.adapter = walletAdapter

        addWalletFab.setOnClickListener { // FAB 클릭 리스너
            showAddWalletDialog()
        }

        loadWallets()
    }

    override fun onResume() {
        super.onResume()
        loadWallets() // 앱이 다시 활성화될 때마다 지갑 목록 갱신 (잔액 반영 위함)
    }

    private fun loadWallets() {
        val walletsArray = getAllWalletsNative()
        walletList.clear()
        walletList.addAll(walletsArray.toList())
        walletAdapter.notifyDataSetChanged()
        Log.d("MainActivity", "Loaded ${walletList.size} wallets.")

        // Empty State UI 가시성 관리
        if (walletList.isEmpty()) {
            walletRecyclerView.visibility = View.GONE
            noWalletsTextView.visibility = View.VISIBLE
        } else {
            walletRecyclerView.visibility = View.VISIBLE
            noWalletsTextView.visibility = View.GONE
        }
    }

    private fun showAddWalletDialog() {
        val dialogView = LayoutInflater.from(this).inflate(R.layout.dialog_add_wallet, null)
        val builder = AlertDialog.Builder(this)
            .setView(dialogView)
            .setTitle("새 지갑 추가")

        val dialog = builder.show()

        val nameEditText = dialogView.findViewById<EditText>(R.id.walletNameEditText)
        val descriptionEditText = dialogView.findViewById<EditText>(R.id.walletDescriptionEditText)
        val addConfirmButton = dialogView.findViewById<Button>(R.id.addWalletConfirmButton)
        val addCancelButton = dialogView.findViewById<Button>(R.id.addWalletCancelButton)

        addConfirmButton.setOnClickListener {
            val name = nameEditText.text.toString().trim()
            val description = descriptionEditText.text.toString().trim()

            if (name.isEmpty()) {
                Toast.makeText(this, "지갑 이름을 입력해주세요.", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            val success = createWalletNative(name, description, 0L) // 초기 잔액은 0

            if (success) {
                Toast.makeText(this, "지갑이 성공적으로 추가되었습니다.", Toast.LENGTH_SHORT).show()
                dialog.dismiss()
                loadWallets() // 목록 갱신
            } else {
                Toast.makeText(this, "지갑 추가 실패.", Toast.LENGTH_SHORT).show()
            }
        }

        addCancelButton.setOnClickListener {
            dialog.dismiss()
        }
    }

    private fun showEditDeleteWalletDialog(wallet: WalletDto) {
        val dialogView = LayoutInflater.from(this).inflate(R.layout.dialog_edit_delete_wallet, null)
        val builder = AlertDialog.Builder(this)
            .setView(dialogView)
            .setTitle("지갑 수정/삭제")

        val dialog = builder.show()

        val nameEditText = dialogView.findViewById<EditText>(R.id.editWalletNameEditText)
        val descriptionEditText = dialogView.findViewById<EditText>(R.id.editWalletDescriptionEditText)
        val updateButton = dialogView.findViewById<Button>(R.id.updateWalletConfirmButton)
        val deleteButton = dialogView.findViewById<Button>(R.id.deleteWalletButton)
        val cancelButton = dialogView.findViewById<Button>(R.id.editWalletCancelButton)

        // 현재 지갑 정보로 UI 채우기
        nameEditText.setText(wallet.name)
        descriptionEditText.setText(wallet.description)

        updateButton.setOnClickListener {
            val newName = nameEditText.text.toString().trim()
            val newDescription = descriptionEditText.text.toString().trim()

            if (newName.isEmpty()) {
                Toast.makeText(this, "지갑 이름을 입력해주세요.", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            // 잔액은 Native에서 재계산하므로, 여기서는 기존 잔액을 유지하거나 0으로 넘겨도 됨
            // 중요: updateWalletNative는 Native에서 잔액을 수정하지 않고 받은 값을 그대로 쓰므로,
            // 트랜잭션으로 인한 잔액 변동을 반영하려면 항상 최신 잔액을 넘겨줘야 함
            // 현재 walletDto의 balance를 그대로 넘기되, recalculateBalance가 명시적으로 호출되므로 큰 문제는 없음
            val success = updateWalletNative(wallet.id, newName, newDescription, wallet.balance)

            if (success) {
                Toast.makeText(this, "지갑이 성공적으로 수정되었습니다.", Toast.LENGTH_SHORT).show()
                dialog.dismiss()
                loadWallets() // 목록 갱신
            } else {
                Toast.makeText(this, "지갑 수정 실패.", Toast.LENGTH_SHORT).show()
            }
        }

        deleteButton.setOnClickListener {
            AlertDialog.Builder(this)
                .setTitle("지갑 삭제")
                .setMessage("정말로 이 지갑을 삭제하시겠습니까? 관련 트랜잭션도 모두 삭제됩니다.")
                .setPositiveButton("삭제") { _, _ ->
                    val success = deleteWalletNative(wallet.id)
                    if (success) {
                        Toast.makeText(this, "지갑이 성공적으로 삭제되었습니다.", Toast.LENGTH_SHORT).show()
                        dialog.dismiss()
                        loadWallets() // 목록 갱신
                    } else {
                        Toast.makeText(this, "지갑 삭제 실패.", Toast.LENGTH_SHORT).show()
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