package com.example.pocketmoneyapp.ui

import android.content.Context
import android.graphics.Color
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.example.pocketmoneyapp.R
import com.example.pocketmoneyapp.data.WalletDto

class WalletAdapter(
    private val context: Context,
    private val wallets: MutableList<WalletDto>,
    private val onWalletClick: (WalletDto) -> Unit, // 일반 클릭 리스너
    private val onWalletLongClick: (WalletDto) -> Boolean // 롱 클릭 리스너 (true 반환 시 이벤트 소비)
) : RecyclerView.Adapter<WalletAdapter.WalletViewHolder>() {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): WalletViewHolder {
        val view = LayoutInflater.from(context).inflate(R.layout.item_wallet, parent, false)
        return WalletViewHolder(view)
    }

    override fun onBindViewHolder(holder: WalletViewHolder, position: Int) {
        val wallet = wallets[position]
        holder.bind(wallet)
    }

    override fun getItemCount(): Int = wallets.size

    inner class WalletViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        private val nameTextView: TextView = itemView.findViewById(R.id.walletNameTextView)
        private val descriptionTextView: TextView = itemView.findViewById(R.id.walletDescriptionTextView)
        private val balanceTextView: TextView = itemView.findViewById(R.id.walletBalanceTextView)

        init {
            // 아이템 클릭 리스너
            itemView.setOnClickListener {
                onWalletClick(wallets[adapterPosition])
            }
            // 아이템 롱 클릭 리스너
            itemView.setOnLongClickListener {
                onWalletLongClick(wallets[adapterPosition])
            }
        }

        fun bind(wallet: WalletDto) {
            nameTextView.text = wallet.name
            descriptionTextView.text = wallet.description

            val formattedBalance = String.format("%,d", wallet.balance)
            balanceTextView.text = "잔액: $formattedBalance 원"

            // 잔액에 따라 텍스트 색상 변경 (예시)
            if (wallet.balance < 0) {
                balanceTextView.setTextColor(Color.parseColor("#F44336")) // 빨간색 (마이너스 잔액)
            } else {
                balanceTextView.setTextColor(Color.parseColor("#3F51B5")) // 파란색 (긍정 잔액)
            }
        }
    }
}