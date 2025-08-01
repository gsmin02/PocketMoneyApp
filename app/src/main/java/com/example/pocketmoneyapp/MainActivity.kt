package com.example.pocketmoneyapp

import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.example.pocketmoneyapp.data.WalletDto
import com.example.pocketmoneyapp.ui.WalletAdapter
import java.io.File
import kotlin.collections.List

class MainActivity : AppCompatActivity() {

    companion object {
        init {
            System.loadLibrary("native_core")
        }
    }

    private external fun initializeNativeDb(dbPath: String)
    private external fun createWalletNative(name: String, description: String, balance: Long): Boolean
    // 받은 Array<WalletDto>를 toList()를 활용해 List<WalletDto>로 변환
    private external fun getAllWalletsNative(): Array<WalletDto>

    // 지갑 수정 및 삭제 JNI 함수 추가
    private external fun updateWalletNative(id: Int, name: String, description: String, balance: Long): Boolean
    private external fun deleteWalletNative(id: Int): Boolean

    private lateinit var walletNameEditText: EditText
    private lateinit var walletDescEditText: EditText
    private lateinit var walletBalanceEditText: EditText
    private lateinit var createWalletButton: Button
    private lateinit var refreshListButton: Button
    private lateinit var walletIdEditText: EditText
    private lateinit var updateWalletButton: Button
    private lateinit var deleteWalletButton: Button
    private lateinit var walletRecyclerView: RecyclerView
    private lateinit var walletAdapter: WalletAdapter
    private val walletList = mutableListOf<WalletDto>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // UI 요소 초기화
        walletNameEditText = findViewById(R.id.walletNameEditText)
        walletDescEditText = findViewById(R.id.walletDescEditText)
        walletBalanceEditText = findViewById(R.id.walletBalanceEditText)
        createWalletButton = findViewById(R.id.createWalletButton)
        refreshListButton = findViewById(R.id.refreshListButton)

        walletIdEditText = findViewById(R.id.walletIdEditText)
        updateWalletButton = findViewById(R.id.updateWalletButton)
        deleteWalletButton = findViewById(R.id.deleteWalletButton)

        // RecyclerView 초기화
        walletRecyclerView = findViewById(R.id.walletRecyclerView)
        walletRecyclerView.layoutManager = LinearLayoutManager(this) // 세로 방향 스크롤 리스트
        walletAdapter = WalletAdapter(walletList)
        walletRecyclerView.adapter = walletAdapter


        // 데이터베이스 초기화
        val dbFileName = "pocketmoney.db"
        val dbPath = applicationContext.filesDir.absolutePath + File.separator + dbFileName
        Log.d("MainActivity", "Database path for native: $dbPath")
        initializeNativeDb(dbPath)

        createWalletButton.setOnClickListener {
            val name = walletNameEditText.text.toString()
            val description = walletDescEditText.text.toString()
            val balanceStr = walletBalanceEditText.text.toString()
            if (name.isBlank() || balanceStr.isBlank()) {
                Log.e("MainActivity", "Wallet name and balance cannot be empty.")
                return@setOnClickListener
            }
            val balance = balanceStr.toLongOrNull() ?: 0L
            val success = createWalletNative(name, description, balance)
            if (success) {
                Log.d("MainActivity", "Wallet '$name' created successfully in native.")
                refreshWalletList()
                walletNameEditText.text.clear()
                walletDescEditText.text.clear()
                walletBalanceEditText.text.clear()
                walletIdEditText.text.clear()
            } else {
                Log.e("MainActivity", "Failed to create wallet in native.")
            }
        }

        updateWalletButton.setOnClickListener {
            val idStr = walletIdEditText.text.toString()
            val id = idStr.toIntOrNull()
            val name = walletNameEditText.text.toString()
            val description = walletDescEditText.text.toString()
            val balanceStr = walletBalanceEditText.text.toString()
            val balance = balanceStr.toLongOrNull() ?: 0L

            if (id != null) {
                val success = updateWalletNative(id, name, description, balance)
                if (success) {
                    Log.d("MainActivity", "Wallet with ID $id updated successfully.")
                    refreshWalletList()
                } else {
                    Log.e("MainActivity", "Failed to update wallet with ID $id.")
                }
            } else {
                Log.e("MainActivity", "Please enter a valid wallet ID.")
            }
        }

        deleteWalletButton.setOnClickListener {
            val idStr = walletIdEditText.text.toString()
            val id = idStr.toIntOrNull()

            if (id != null) {
                val success = deleteWalletNative(id)
                if (success) {
                    Log.d("MainActivity", "Wallet with ID $id deleted successfully.")
                    refreshWalletList()
                } else {
                    Log.e("MainActivity", "Failed to delete wallet with ID $id.")
                }
            } else {
                Log.e("MainActivity", "Please enter a valid wallet ID.")
            }
        }

        refreshListButton.setOnClickListener {
            refreshWalletList()
        }

        refreshWalletList()
    }

    private fun refreshWalletList() {
        val wallets: List<WalletDto> = getAllWalletsNative().toList()
        walletAdapter.updateWallets(wallets) // 어댑터에 데이터 전달 및 갱신
        Log.d("MainActivity", "Wallet list refreshed with ${wallets.size} items.")
    }
}