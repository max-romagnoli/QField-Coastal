/**
 * QFieldPositioningService.java
 * @author  Mathieu Pellerin - <mathieu@opengis.ch>
 */
/*
 Copyright (c) 2024, Mathieu Pellerin <mathieu@opengis.ch>
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 * Neither the name of the  Marco Bernasocchi <marco@opengis.ch> nor the
 names of its contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY Marco Bernasocchi <marco@opengis.ch> ''AS IS'' AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL Marco Bernasocchi <marco@opengis.ch> BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package ch.opengis.qfield;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.util.Log;
import org.qtproject.qt.android.bindings.QtService;

public class QFieldPositioningService extends QtService {

    private NotificationManager notificationManager;
    private NotificationChannel notificationChannel;

    private final String CHANNEL_ID = "qfield_service_02";
    private final int NOTIFICATION_ID = 102;

    private static QFieldPositioningService instance = null;
    public static QFieldPositioningService getInstance() {
        return instance;
    }

    public static void startQFieldPositioningService(Context context) {
        Log.v("QFieldPositioningService", "Starting QFieldPositioningService");
        Intent intent = new Intent(context, QFieldPositioningService.class);
        context.startForegroundService(intent);
    }

    public static void stopQFieldPositioningService(Context context) {
        Log.v("QFieldPositioningService", "Stopping QFieldPositioningService");
        Intent intent = new Intent(context, QFieldPositioningService.class);
        context.stopService(intent);
    }

    public static void triggerShowNotification(String message) {
        if (getInstance() != null) {
            getInstance().showNotification(message);
        } else {
            Log.v("QFieldPositioningService",
                  "Showing message failed, no instance available.");
        }
    }

    public static void triggerCloseNotification() {
        if (getInstance() != null) {
            getInstance().closeNotification();
        } else {
            Log.v("QFieldPositioningService",
                  "Closing message failed, no instance available.");
        }
    }

    @Override
    public void onCreate() {
        Log.v("QFieldPositioningService", "onCreate triggered");
        super.onCreate();

        if (getInstance() != null) {
            Log.v("QFieldPositioningService",
                  "service already running, aborting.");
            stopSelf();
            return;
        }
    }

    @Override
    public void onDestroy() {
        Log.v("QFieldPositioningService", "onDestroy triggered");
        notificationManager.cancel(NOTIFICATION_ID);
        super.onDestroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.v("QFieldPositioningService", "onStartCommand triggered");
        int ret = super.onStartCommand(intent, flags, startId);

        if (instance != null) {
            stopSelf();
            return START_NOT_STICKY;
        }

        instance = this;

        notificationManager =
            (NotificationManager)getSystemService(NOTIFICATION_SERVICE);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            notificationChannel = new NotificationChannel(
                CHANNEL_ID, "QField", NotificationManager.IMPORTANCE_DEFAULT);
            notificationChannel.setDescription("QField positioning");
            notificationChannel.setImportance(
                NotificationManager.IMPORTANCE_LOW);
            notificationChannel.enableLights(false);
            notificationChannel.enableVibration(false);
            notificationManager.createNotificationChannel(notificationChannel);
        }

        Notification.Builder builder =
            new Notification.Builder(this)
                .setSmallIcon(R.drawable.qfield_logo)
                .setWhen(System.currentTimeMillis())
                .setOngoing(true)
                .setContentTitle("QField")
                .setContentText("Positioning service running");

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            builder.setChannelId(CHANNEL_ID);
        }

        Notification notification = builder.build();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            startForeground(NOTIFICATION_ID, notification,
                            ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION);
        } else {
            startForeground(NOTIFICATION_ID, notification);
        }

        return START_STICKY;
    }

    public void showNotification(String contentText) {
        Notification.Builder builder = new Notification.Builder(this)
                                           .setSmallIcon(R.drawable.qfield_logo)
                                           .setWhen(System.currentTimeMillis())
                                           .setOngoing(true)
                                           .setContentTitle("QField")
                                           .setContentText(contentText);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            builder.setChannelId(CHANNEL_ID);
        }

        Notification notification = builder.build();
        notificationManager.notify(NOTIFICATION_ID, notification);
    }

    public void closeNotification() {
        notificationManager.cancel(NOTIFICATION_ID);
    }
}
