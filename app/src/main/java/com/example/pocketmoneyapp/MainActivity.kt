package com.example.pocketmoneyapp

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.ui.Modifier
import android.util.Log

class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            Surface(
                modifier = Modifier.fillMaxSize(),
                color = MaterialTheme.colorScheme.background
            ) {
                val nativeMessage = stringFromNativeCore()
                Log.d("MainActivity", "Message from Native: $nativeMessage")
                Text(text = "Hello from Kotlin and Native Core: $nativeMessage")
            }
        }
    }

    /**
     * A native method that is implemented by the 'native_core' native library,
     * which is packaged with this application.
     */
    external fun stringFromNativeCore(): String

    companion object {
        // Used to load the 'native_core' library on application startup.
        init {
            System.loadLibrary("native_core")
        }
    }
}