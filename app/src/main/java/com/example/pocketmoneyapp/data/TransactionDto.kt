package com.example.pocketmoneyapp.data

enum class TransactionType(val value: Int) {
    INCOME(0),
    EXPENSE(1);

    companion object {
        fun fromInt(value: Int) = values().first { it.value == value }
    }
}

data class TransactionDto(
    val id: Int,
    val walletId: Int,
    val amount: Long,
    val description: String,
    val type: Int,
    val transactionDate: String
)