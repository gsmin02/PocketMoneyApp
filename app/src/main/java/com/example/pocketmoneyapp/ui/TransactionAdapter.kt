package com.example.pocketmoneyapp.ui

import android.content.Context
import android.graphics.Color
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.example.pocketmoneyapp.R
import com.example.pocketmoneyapp.data.TransactionDto
import com.example.pocketmoneyapp.data.TransactionType

class TransactionAdapter(
    private val context: Context,
    private val transactions: MutableList<TransactionDto>,
    private val onTransactionClick: (TransactionDto) -> Unit // 클릭 리스너 추가
) : RecyclerView.Adapter<TransactionAdapter.TransactionViewHolder>() {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): TransactionViewHolder {
        val view = LayoutInflater.from(context).inflate(R.layout.item_transaction, parent, false)
        return TransactionViewHolder(view)
    }

    override fun onBindViewHolder(holder: TransactionViewHolder, position: Int) {
        val transaction = transactions[position]
        holder.bind(transaction)
    }

    override fun getItemCount(): Int = transactions.size

    inner class TransactionViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        private val descriptionTextView: TextView = itemView.findViewById(R.id.transactionDescriptionTextView)
        private val amountTextView: TextView = itemView.findViewById(R.id.transactionAmountTextView)
        private val dateTextView: TextView = itemView.findViewById(R.id.transactionDateTextView)

        init {
            itemView.setOnClickListener {
                // 아이템 클릭 시 람다 호출
                onTransactionClick(transactions[adapterPosition])
            }
        }

        fun bind(transaction: TransactionDto) {
            descriptionTextView.text = transaction.description
            dateTextView.text = transaction.transactionDate

            val formattedAmount = String.format("%,d", transaction.amount)

            // 타입에 따라 색상 변경
            if (transaction.type == TransactionType.INCOME.value) {
                amountTextView.text = "+ $formattedAmount 원"
                amountTextView.setTextColor(Color.parseColor("#4CAF50")) // 녹색 (수입)
            } else {
                amountTextView.text = "- $formattedAmount 원"
                amountTextView.setTextColor(Color.parseColor("#F44336")) // 빨간색 (지출)
            }
        }
    }
}