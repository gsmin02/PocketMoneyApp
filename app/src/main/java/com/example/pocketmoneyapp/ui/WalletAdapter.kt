package com.example.pocketmoneyapp.ui

import android.content.Context
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
    private val onItemClick: (WalletDto) -> Unit
) : RecyclerView.Adapter<WalletAdapter.WalletViewHolder>() {

    class WalletViewHolder(view: View) : RecyclerView.ViewHolder(view) {
        val nameTextView: TextView = view.findViewById(R.id.walletName)
        val descriptionTextView: TextView = view.findViewById(R.id.walletDescription)
        val balanceTextView: TextView = view.findViewById(R.id.walletBalance)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): WalletViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.list_item_wallet, parent, false)
        return WalletViewHolder(view)
    }

    override fun onBindViewHolder(holder: WalletViewHolder, position: Int) {
        val wallet = wallets[position]
        holder.nameTextView.text = wallet.name
        holder.descriptionTextView.text = wallet.description
        holder.balanceTextView.text = String.format("%,d원", wallet.balance) // 금액 포맷팅

        // 항목 클릭 시 리스너 호출
        holder.itemView.setOnClickListener {
            onItemClick(wallet)
        }
    }

    override fun getItemCount(): Int = wallets.size

    fun updateWallets(newWallets: List<WalletDto>) {
        wallets.clear()
        wallets.addAll(newWallets)
        notifyDataSetChanged()
    }
}