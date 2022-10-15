#include "common.h"
#include "app/app.h"
#include "led_app/led_app.h"
#include "uart_app/uart_app.h"
#include "inet_app/tcp_client/tcp_client.h"
#include "inet_app/tcp_server/tcp_server.h"


#if INET_TCP_SERVER
int tcp_server_part(app_p app)
{
    msg_init(&app->msg_id);                      // 消息队列 初始化
    // app_led_init(&app->led, app->msg_id);         // led设备 初始化
    // tty_init(&app->uart, app->msg_id);            // uart设备 初始化
    tcp_server_init(&app->server, app->msg_id);

    // thread start
    app_start(app);                            // 启动 app
    // app_led_start(&app->led);                    // 启动 led
    // tty_start(&app->uart);                       // 启动 uart
    tcp_server_start(&app->server);            // 启动server    

    pthread_join(app->app_td, NULL);             // 链接线程 app_thread
    // pthread_join(app->uart.recive_td, NULL);     // 链接线程 uart_read_thread
    // pthread_join(app->uart.handle_td, NULL);     // 链接线程 urat_handle_thread
    // pthread_join(app->uart.send_td, NULL);       // 链接线程 uart_write_thread
    // pthread_join(app->led.handle_td, NULL);      // 链接线程 led_handle_thread  
    pthread_join(app->server.mng_tid, NULL);
    pthread_join(app->server.keep_tid, NULL);

    // thread stop
    // tty_stop(&app->uart);                // 停止运行线程，那三兄弟
    // app_led_stop(&app->led);
    app_stop(app);
    tcp_server_stop(&app->server);            // 启动server
   
    // deinit
    // tty_deinit(&app->uart);                      // 去初始化线程
    // app_led_deinit(&app->led);
    msg_deinit(app->msg_id);
    tcp_server_deinit(&app->server);            // 启动server

    return 0;
}

#else
int tcp_client_part(app_p app)
{
    msg_init(&app->msg_id);                      // 消息队列 初始化
    app_led_init(&app->led, app->msg_id);         // led设备 初始化
    tty_init(&app->uart, app->msg_id);            // uart设备 初始化
    tcp_client_init(&app->client, app->msg_id);

    // thread start
    app_start(app);                            // 启动 app
    app_led_start(&app->led);                    // 启动 led
    tty_start(&app->uart);                       // 启动 uart
    tcp_client_start(&app->client);          // 启动client


    pthread_join(app->app_td, NULL);             // 链接线程 app_thread
    pthread_join(app->uart.recive_td, NULL);     // 链接线程 uart_read_thread
    pthread_join(app->uart.handle_td, NULL);     // 链接线程 urat_handle_thread
    pthread_join(app->uart.send_td, NULL);       // 链接线程 uart_write_thread
    pthread_join(app->led.handle_td, NULL);      // 链接线程 led_handle_thread
    pthread_join(app->client.connect_tid, NULL);
    pthread_join(app->client.keep_tid, NULL);
    pthread_join(app->client.commit_tid, NULL);

    // thread stop
    tty_stop(&app->uart);                // 停止运行线程
    app_led_stop(&app->led);
    app_stop(app);
    tcp_client_stop(&app->client);          // 启动client
  
    // deinit
    tty_deinit(&app->uart);                      // 去初始化线程
    app_led_deinit(&app->led);
    msg_deinit(app->msg_id);
    tcp_client_deinit(&app->client);          // 启动client

    return 0;
}

#endif

int main()
{
    app_t app;
    
#if INET_TCP_SERVER
    return tcp_server_part(&app);
#else
    return tcp_client_part(&app);
#endif
}
