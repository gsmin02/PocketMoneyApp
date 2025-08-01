package com.example.pocketmoneyapp.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.example.pocketmoneyapp.R
import com.example.pocketmoneyapp.data.WalletDto // WalletDto 임포트

class WalletAdapter(private val wallets: MutableList<WalletDto>) :
    RecyclerView.Adapter<WalletAdapter.WalletViewHolder>() {

    class WalletViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        val walletIdTextView: TextView = itemView.findViewById(R.id.walletIdTextView)
        val walletNameTextView: TextView = itemView.findViewById(R.id.walletNameTextView)
        val walletDescriptionTextView: TextView = itemView.findViewById(R.id.walletDescriptionTextView)
        val walletBalanceTextView: TextView = itemView.findViewById(R.id.walletBalanceTextView)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): WalletViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.list_item_wallet, parent, false)
        return WalletViewHolder(view)
    }

    override fun onBindViewHolder(holder: WalletViewHolder, position: Int) {
        val wallet = wallets[position]
        holder.walletIdTextView.text = "ID: ${wallet.id}"
        holder.walletNameTextView.text = wallet.name
        holder.walletDescriptionTextView.text = wallet.description
        holder.walletBalanceTextView.text = "Balance: ${wallet.balance}"

        // TODO: 나중에 지갑 항목 클릭 시 수정/삭제 로직 추가 가능
        // holder.itemView.setOnClickListener {
        //     // 클릭 이벤트 처리
        // }
    }

    override fun getItemCount(): Int {
        return wallets.size
    }

    // 데이터 업데이트 메서드 (외부에서 데이터 변경 시 호출)
    fun updateWallets(newWallets: List<WalletDto>) {
        wallets.clear()
        wallets.addAll(newWallets)
        notifyDataSetChanged()
    }
}