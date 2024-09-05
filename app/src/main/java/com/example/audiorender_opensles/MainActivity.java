package com.example.audiorender_opensles;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;
import android.os.Environment;
import android.content.Context;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.FileNotFoundException;

import com.example.audiorender_opensles.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'audiorender_opensles' library on application startup.
    static {
        System.loadLibrary("audiorender_opensles");
    }


    private ActivityMainBinding binding;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());


        // Example of a call to a native method
        writeFileToPrivateStorage(R.raw.output, "output.pcm");
        //获取私有存储路径 + pcm文件名
        String path = this.getApplicationContext().getFilesDir().toString() + "/output.pcm";


        PlayAudio(path);
    }

    //将文件复制到应用私有存储
    public void writeFileToPrivateStorage(int fromFile, String toFile) {
        InputStream is = getResources().openRawResource(fromFile);
        int bytes_read;
        byte[] buffer = new byte[4096];
        try {
            FileOutputStream fos = openFileOutput(toFile, Context.MODE_PRIVATE);

            while ((bytes_read = is.read(buffer)) != -1) {
                fos.write(buffer, 0, bytes_read); // 写入文件
            }

            fos.close();
            is.close();

        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    /**
     * A native method that is implemented by the 'audiorender_opensles' native library,
     * which is packaged with this application.
     */
    public native void PlayAudio(String url);


}