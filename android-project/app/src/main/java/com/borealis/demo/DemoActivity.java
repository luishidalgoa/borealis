package com.borealis.demo;
import android.os.Bundle;

import org.libsdl.app.BorealisHandler;
import org.libsdl.app.PlatformUtils;
import org.libsdl.app.SDLActivity;

public class DemoActivity extends SDLActivity
{


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Currently we use handler to receive brightness changes from borealis
        PlatformUtils.borealisHandler = new BorealisHandler();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        // Android does not recommend using exit(0) directly,
        // but borealis heavily uses static variables,
        // which can cause some problems when reloading the program.

        // Force exit of the app until Borealis supports recreating its static state.
        System.exit(0);
    }

    @Override
    protected String[] getLibraries() {
        // Load SDL3 and the Borealis demo app.
        return new String[] {
                "SDL3",
                "borealis_demo"
        };
    }

}
