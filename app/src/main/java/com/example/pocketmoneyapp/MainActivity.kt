package com.example.pocketmoneyapp

import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.example.pocketmoneyapp.data.TransactionDto
import com.example.pocketmoneyapp.data.WalletDto
import com.example.pocketmoneyapp.ui.WalletAdapter
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import kotlin.collections.List

import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts

class MainActivity : AppCompatActivity() {

    companion object {
        init {
            System.loadLibrary("native_core")
        }
    }

    private external fun initializeNativeDb(dbPath: String)
    private external fun createWalletNative(name: String, description: String, balance: Long): Boolean
    private external fun getAllWalletsNative(): Array<WalletDto>
    private external fun updateWalletNative(id: Int, name: String, description: String, balance: Long): Boolean
    private external fun deleteWalletNative(id: Int): Boolean
    private external fun getWalletByIdNative(id: Int): WalletDto

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

    private val transactionActivityLauncher: ActivityResultLauncher<Intent> =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
            if (result.resultCode == RESULT_OK) {
                val shouldRefreshWallets = result.data?.getBooleanExtra("SHOULD_REFRESH_WALLETS", false) ?: false
                if (shouldRefreshWallets) {
                    refreshWalletList()
                    Log.d("MainActivity", "Wallets refreshed due to TransactionListActivity result.")
                }
            }
        }

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
        walletAdapter = WalletAdapter(this, walletList) { wallet ->
            val intent = Intent(this, TransactionListActivity::class.java).apply {
                putExtra("WALLET_ID", wallet.id)
                putExtra("WALLET_NAME", wallet.name)
                putExtra("WALLET_BALANCE", wallet.balance)
            }
            transactionActivityLauncher.launch(intent)
        }
        walletRecyclerView.layoutManager = LinearLayoutManager(this)
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

    override fun onResume() {
        super.onResume()
        // MainActivity로 돌아올 때마다 지갑 목록을 새로고침하여 최신 잔액 반영
        refreshWalletList()
    }

    private fun refreshWalletList() {
        val walletsArray = getAllWalletsNative()
        walletList.clear()
        walletList.addAll(walletsArray.toList())
        walletAdapter.notifyDataSetChanged()
        Log.d("MainActivity", "Wallet list refreshed. Loaded ${walletList.size} wallets.")
    }
}