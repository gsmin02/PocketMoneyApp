package com.example.pocketmoneyapp

import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.example.pocketmoneyapp.data.WalletDto
import java.io.File

class MainActivity : AppCompatActivity() {

    companion object {
        init {
            System.loadLibrary("native_core")
        }
    }

    private external fun initializeNativeDb(dbPath: String)
    private external fun createWalletNative(name: String, description: String, balance: Long): Boolean
    private external fun getAllWalletNamesNative(): String // 모든 지갑 이름을 문자열로 반환 (간단한 테스트용)

    private lateinit var walletNameEditText: EditText
    private lateinit var walletDescEditText: EditText
    private lateinit var walletBalanceEditText: EditText
    private lateinit var createWalletButton: Button
    private lateinit var walletListTextView: TextView
    private lateinit var refreshListButton: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // UI 요소 초기화
        walletNameEditText = findViewById(R.id.walletNameEditText)
        walletDescEditText = findViewById(R.id.walletDescEditText)
        walletBalanceEditText = findViewById(R.id.walletBalanceEditText)
        createWalletButton = findViewById(R.id.createWalletButton)
        walletListTextView = findViewById(R.id.walletListTextView)
        refreshListButton = findViewById(R.id.refreshListButton)

        val dbFileName = "pocketmoney.db"
        // 앱의 내부 저장소 경로 (앱 제거 시 같이 삭제됨)
        val dbPath = applicationContext.filesDir.absolutePath + File.separator + dbFileName
        Log.d("MainActivity", "Database path for native: $dbPath")
        initializeNativeDb(dbPath)


        // 지갑 생성 버튼 클릭 리스너
        createWalletButton.setOnClickListener {
            val name = walletNameEditText.text.toString()
            val description = walletDescEditText.text.toString()
            val balanceStr = walletBalanceEditText.text.toString()

            if (name.isBlank() || balanceStr.isBlank()) {
                Log.e("MainActivity", "Wallet name and balance cannot be empty.")
                walletListTextView.text = "Error: Name and balance required!"
                return@setOnClickListener
            }

            val balance = balanceStr.toLongOrNull() ?: 0L

            val success = createWalletNative(name, description, balance)
            if (success) {
                Log.d("MainActivity", "Wallet created successfully in native.")
                walletListTextView.text = "Wallet '$name' created successfully!\n"
                // 생성 후 목록 새로고침
                refreshWalletList()
                // 입력 필드 초기화
                walletNameEditText.text.clear()
                walletDescEditText.text.clear()
                walletBalanceEditText.text.clear()
            } else {
                Log.e("MainActivity", "Failed to create wallet in native.")
                walletListTextView.text = "Failed to create wallet."
            }
        }

        refreshListButton.setOnClickListener {
            refreshWalletList()
        }

        // 앱 시작 시 초기 목록 로드
        refreshWalletList()
    }

    private fun refreshWalletList() {
        val walletsInfo = getAllWalletNamesNative()
        walletListTextView.text = walletsInfo
        Log.d("MainActivity", "Wallet list refreshed: \n$walletsInfo")
    }
}