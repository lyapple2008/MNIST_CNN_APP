package com.example.beyoung.handwrittendigit;

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.os.AsyncTask;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "HandwrittenDigit";
    private final int IMG_H = 28;
    private final int IMG_W = 28;
    private Button inputBtn;
    private Button clearBtn;
    private DoodleView doodleView;
    private TextView tv;
    private AssetManager mgr;
    private String recogResult;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native void initCaffe2(AssetManager mgr);
    public native String recognitionFromCaffe2(int h, int w, int[] data);
    public native String stringFromJNI();

    private class SetupNeuralNetwork extends AsyncTask<Void, Void, Void> {
        @Override
        protected Void doInBackground(Void... voids) {
            try {
                initCaffe2(mgr);
                Log.d(TAG, "Neural net loaded! Inferring...");
            } catch (Exception e) {
                Log.d(TAG, "Could't load neural network.");
            }
            return null;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mgr = getResources().getAssets();
        new SetupNeuralNetwork().execute();

        // Example of a call to a native method
        tv = (TextView) findViewById(R.id.resultText);
        tv.setText(stringFromJNI());

        doodleView = findViewById(R.id.doodleView);

        inputBtn = findViewById(R.id.inputBtn);
        inputBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //doodleView.saveBitmap(doodleView);
                //Toast.makeText(getApplicationContext(), "Save Bitmap", Toast.LENGTH_SHORT).show();

                Bitmap bitmap = doodleView.getBitmap();
                Bitmap scaledBitmp = Bitmap.createScaledBitmap(bitmap, IMG_H, IMG_W, false);
                int[] pixels = new int[IMG_H * IMG_W];
                scaledBitmp.getPixels(pixels, 0, scaledBitmp.getWidth(), 0, 0, IMG_W, IMG_H);
                recogResult = recognitionFromCaffe2(IMG_H, IMG_W, pixels);
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        tv.setText(recogResult);
                    }
                });
            }
        });
        clearBtn = findViewById(R.id.clearBtn);
        clearBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                doodleView.reset();
            }
        });
    }



}
