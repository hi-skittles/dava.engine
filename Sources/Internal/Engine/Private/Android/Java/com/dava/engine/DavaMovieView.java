package com.dava.engine;

import android.graphics.Color;
import android.media.MediaPlayer;
import android.view.View;
import android.widget.VideoView;
import android.widget.RelativeLayout;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

// Duplicates enum eMovieScalingMode declared in UI/IMovieViewControl.h
class eMovieScalingMode
{
    static final int scalingModeNone = 0;
    static final int scalingModeAspectFit = 1;
    static final int scalingModeAspectFill = 2;
    static final int scalingModeFill = 3;
};

class eMoviePlayingState
{
    static final int stateStopped = 0;
    static final int stateLoading = 1;
    static final int statePaused = 2;
    static final int statePlaying = 3;
};

final class DavaMovieView implements MediaPlayer.OnCompletionListener,
                                     MediaPlayer.OnPreparedListener,
                                     MediaPlayer.OnErrorListener
{
    // Duplicates enum MovieViewControl::eAction declared in UI/Private/Android/MovieViewControlAndroid.h
    class eAction
    {
        static final int ACTION_PLAY = 0;
        static final int ACTION_PAUSE = 1;
        static final int ACTION_RESUME = 2;
        static final int ACTION_STOP = 3;
    }

    // About java volatile https://docs.oracle.com/javase/tutorial/essential/concurrency/atomic.html
    private volatile long movieviewBackendPointer = 0;
    private DavaSurfaceView surfaceView = null;
    // VideoView is located inside RelativeLayout which in turn is added into view hierarchy
    private RelativeLayout movieViewLayout = null;
    private VideoView nativeMovieView = null;

    // Properties that have been set in DAVA::Engine thread and waiting to apply to MovieView
    private MovieViewProperties properties = new MovieViewProperties();

    // Some properties that reflect MovieView current properties, accessed in UI thread
    private float x;
    private float y;
    private float width;
    private float height;
    private int scaleMode = 0;
    private boolean movieLoaded = false;        // Movie file is succesfully decoded and loaded
    private boolean playAfterLoaded = false;    // Flag that tells to play movie after being loaded
    private String movieFile = "";

    // Flags accessed in dava main thread (where cpp code lives)
    private int playingState = eMoviePlayingState.stateStopped; // Movie playing state
    private boolean canPlay = false;    // openMovie method has been called and movie file exists

    public static native void nativeReleaseWeakPtr(long backendPointer);

    private class MovieViewProperties
    {
        MovieViewProperties() {}
        MovieViewProperties(MovieViewProperties other)
        {
            x = other.x;
            y = other.y;
            width = other.width;
            height = other.height;
            visible = other.visible;
            moviePath = other.moviePath;
            scaleMode = other.scaleMode;
            action = other.action;

            createNew = other.createNew;
            anyPropertyChanged = other.anyPropertyChanged;
            rectChanged = other.rectChanged;
            visibleChanged = other.visibleChanged;
            movieChanged = other.movieChanged;
            actionChanged = other.actionChanged;
        }

        float x;
        float y;
        float width;
        float height;
        boolean visible;
        String moviePath;
        int scaleMode;
        int action;

        boolean createNew;
        boolean anyPropertyChanged;
        boolean rectChanged;
        boolean visibleChanged;
        boolean movieChanged;
        boolean actionChanged;

        void clearChangedFlags()
        {
            createNew = false;
            anyPropertyChanged = false;
            rectChanged = false;
            visibleChanged = false;
            movieChanged = false;
            actionChanged = false;
        }
    }

    public DavaMovieView(DavaSurfaceView view, long backendPointer)
    {
        movieviewBackendPointer = backendPointer;
        surfaceView = view;

        properties.createNew = true;
        properties.anyPropertyChanged = true;
    }

    void release()
    {
        DavaActivity.commandHandler().post(new Runnable() {
            @Override public void run()
            {
                releaseNativeControl();
            }
        });
    }

    void setRect(float x, float y, float width, float height)
    {
        boolean changed = properties.x != x || properties.y != y ||
                          properties.width != width || properties.height != height; 
        if (changed)
        {
            properties.x = x;
            properties.y = y;
            properties.width = width;
            properties.height = height;
            properties.rectChanged = true;
            properties.anyPropertyChanged = true;
        }
    }

    void setVisible(boolean visible)
    {
        if (properties.visible != visible)
        {
            properties.visible = visible;
            properties.visibleChanged = true;
            properties.anyPropertyChanged = true;
            if (!visible)
            {
                // Immediately hide native control
                DavaActivity.commandHandler().post(new Runnable() {
                    @Override public void run()
                    {
                        if (nativeMovieView != null)
                        {
                            setNativeVisible(false);
                        }
                    }
                });
            }
        }
    }

    void openMovie(String moviePath, int scaleMode)
    {
        canPlay = false;

        String path = prepareMovieFile(moviePath);
        if (path != null)
        {
            canPlay = true;

            properties.moviePath = path;
            properties.scaleMode = scaleMode;
            properties.movieChanged = true;
            properties.anyPropertyChanged = true;
        }
    }

    void doAction(int action)
    {
        if (canPlay)
        {
            // Game does not take into account that video playback can take some time after Play() has been called
            // So assume movie is playing under following conditions:
            //  - movie is really playing
            //  - game has called Play() method after openMovie
            properties.action = action;
            properties.actionChanged = true;
            properties.anyPropertyChanged = true;
        }
    }

    int getState()
    {
        return playingState;
    }

    void update()
    {
        if (properties.anyPropertyChanged)
        {
            final MovieViewProperties props = new MovieViewProperties(properties);
            DavaActivity.commandHandler().post(new Runnable() {
                @Override public void run()
                {
                    processProperties(props);
                }
            });
            properties.clearChangedFlags();
        }
    }

    void processProperties(MovieViewProperties props)
    {
        if (props.createNew)
        {
            createNativeControl();
        }
        if (props.anyPropertyChanged)
        {
            applyChangedProperties(props);
        }
    }

    void createNativeControl()
    {
        nativeMovieView = new VideoView(DavaActivity.instance());
        
        nativeMovieView.setOnCompletionListener(this);
        nativeMovieView.setOnPreparedListener(this);
        nativeMovieView.setOnErrorListener(this);
        nativeMovieView.setBackgroundColor(Color.BLACK);

        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                                                    RelativeLayout.LayoutParams.MATCH_PARENT,
                                                    RelativeLayout.LayoutParams.MATCH_PARENT);
        params.alignWithParent = true;

        RelativeLayout layout = new RelativeLayout(DavaActivity.instance());
        layout.addView(nativeMovieView, params);

        movieViewLayout = layout;

        layout.clearAnimation();
        nativeMovieView.clearAnimation();
        nativeMovieView.setZOrderOnTop(true);

        surfaceView.addControl(movieViewLayout);
    }

    void releaseNativeControl()
    {
        nativeReleaseWeakPtr(movieviewBackendPointer);
        movieviewBackendPointer = 0;

        if (nativeMovieView != null)
        {
            surfaceView.removeControl(movieViewLayout);
            movieViewLayout = null;
            nativeMovieView = null;
        }
    }

    void applyChangedProperties(MovieViewProperties props)
    {
        if (props.visibleChanged)
            setNativeVisible(props.visible);
        if (props.rectChanged)
        {
            x = props.x;
            y = props.y;
            width = props.width;
            height = props.height;
            setNativePositionAndSize(x, y, width, height);
        }
        if (props.movieChanged)
        {
            scaleMode = props.scaleMode;
            movieFile = props.moviePath;
            nativeMovieView.setVideoPath(movieFile);

            playingState = eMoviePlayingState.stateLoading;
            movieLoaded = false;
            playAfterLoaded = false;
        }
        if (props.actionChanged)
        {
            if (movieLoaded)
            {
                switch (props.action)
                {
                case eAction.ACTION_PLAY:
                    playingState = eMoviePlayingState.statePlaying;
                    nativeMovieView.start();
                    break;
                case eAction.ACTION_PAUSE:
                    playingState = eMoviePlayingState.statePaused;
                    nativeMovieView.pause();
                    break;
                case eAction.ACTION_RESUME:
                    playingState = eMoviePlayingState.statePlaying;
                    nativeMovieView.start();
                    break;
                case eAction.ACTION_STOP:
                    playingState = eMoviePlayingState.stateStopped;
                    nativeMovieView.seekTo(0);
                    nativeMovieView.pause();
                    break;
                default:
                    return;
                }
            }
            playAfterLoaded = !movieLoaded && props.action == eAction.ACTION_PLAY;
        }
    }

    void setNativePositionAndSize(float x, float y, float width, float height)
    {
        surfaceView.positionControl(movieViewLayout, x, y, width, height);
    }

    void setNativeVisible(boolean visible)
    {
        nativeMovieView.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    void tellPlayingState(final int state)
    {
        DavaActivity.commandHandler().post(new Runnable() {
            @Override public void run()
            {
                playingState = state;
            }
        });
    }

    String prepareMovieFile(String path)
    {
        File f = new File(path);
        if (!f.exists())
        {
            // It seems that file in assets, so try to copy it to doc folder
            // as VideoView cannot load movie from assets
            try {
                File targetFile = File.createTempFile("movie", null, null);
                targetFile.deleteOnExit();

                InputStream is = DavaActivity.instance().getAssets().open("Data/" + path);
                OutputStream os = new FileOutputStream(targetFile);

                int nread = 0;
                byte[] buffer = new byte[4096];
                while ((nread = is.read(buffer)) != -1)
                {
                    os.write(buffer, 0, nread);
                }
                os.close();
                return targetFile.getAbsolutePath();
            } catch (Exception e) {
                DavaLog.e(DavaActivity.LOG_TAG, "DavaMovieView: failed to extract file from assets: " + e.toString());
            }
        }
        else
        {
            // File is not in assets, so return path as is
            return path;
        }
        return null;
    }

    // MediaPlayer.OnCompletionListener interface
    @Override
    public void onCompletion(MediaPlayer mplayer)
    {
        tellPlayingState(eMoviePlayingState.stateStopped);
    }

    // MediaPlayer.OnPreparedListener interface
    @Override
    public void onPrepared(MediaPlayer mplayer)
    {
        int videoWidth = mplayer.getVideoWidth();
        int videoHeight = mplayer.getVideoHeight();

        if (videoWidth != 0 && videoHeight != 0)
        {
            int w = (int) width;
            int h = (int) height;

            float xFactor = videoWidth / width;
            float yFactor = videoHeight / height;
            RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(0, 0);
            switch (scaleMode) {
                case eMovieScalingMode.scalingModeAspectFit:
                    if (xFactor > yFactor) {
                        params.width = w;
                        params.height = videoHeight * w / videoWidth;
                        params.topMargin = (h - params.height) / 2;
                        params.leftMargin = 0;
                    } else {
                        params.height = h;
                        params.width = videoWidth * h / videoHeight;
                        params.leftMargin = (w - params.width) / 2;
                        params.topMargin = 0;
                    }
                    break;
                case eMovieScalingMode.scalingModeAspectFill:
                    if (xFactor > yFactor) {
                        params.height = h;
                        params.width = videoWidth * h / videoHeight;
                        params.leftMargin = params.rightMargin = (w - params.width) / 2;
                        params.topMargin = 0;
                        params.bottomMargin = 0;
                    } else {
                        params.width = w;
                        params.height = videoHeight * w / videoWidth;
                        params.leftMargin = 0;
                        params.rightMargin = 0;
                        params.topMargin = (h - params.height) / 2;
                        params.bottomMargin = params.topMargin;
                    }
                    break;
                case eMovieScalingMode.scalingModeFill:
                    params.rightMargin = 0;
                    params.bottomMargin = 0;
                    params.width = w;
                    params.height = h;
                    break;
                case eMovieScalingMode.scalingModeNone:
                default:
                    params.width = videoWidth;
                    params.height = videoHeight;
                    params.leftMargin = (w - params.width) / 2;
                    params.topMargin = (h - params.height) / 2;
                    break;
            }

            setNativePositionAndSize(x, y, params.width, params.height);
            nativeMovieView.setLayoutParams(params);
        }

        movieLoaded = true;
        if (playAfterLoaded)
        {
            playAfterLoaded = false;
            doAction(eAction.ACTION_PLAY);
        }
        else
        {
            doAction(eAction.ACTION_STOP);
        }
    }

    // MediaPlayer.OnErrorListener interface
    @Override
    public boolean onError(MediaPlayer mplayer, int what, int extra)
    {
        DavaLog.e(DavaActivity.LOG_TAG, String.format("DavaMovieView.onError: file='%s', what=%d, extra=%d", movieFile, what, extra));

        tellPlayingState(eMoviePlayingState.stateStopped);
        // Return true to prevent showing error dialog on error
        return true;
    }
}
