#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include <uros_network_interfaces.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rmw_microxrcedds_c/config.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <foxglove_msgs/msg/raw_audio.h>
#include "i2s_std.h"

#ifdef CONFIG_MICRO_ROS_ESP_XRCE_DDS_MIDDLEWARE
#include <rmw_microros/rmw_microros.h>
#endif

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Aborting.\n",__LINE__,(int)temp_rc);vTaskDelete(NULL);}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Continuing.\n",__LINE__,(int)temp_rc);}}

rcl_publisher_t publisher;
QueueHandle_t xAudioQueue;
bool isQueueReady;
foxglove_msgs__msg__RawAudio msg;

void timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{
	processed_audio_t* processed_data = NULL; // Allocate memory for processed audio buffer;
	RCLC_UNUSED(last_call_time);
	if (timer != NULL) {
		// 1. Fetch synchronized epoch time in milliseconds and nanoseconds
		int64_t epoch_ms = rmw_uros_epoch_millis();
		int64_t epoch_ns = rmw_uros_epoch_nanos();

		// 2. Assign values to your ROS2 message header stamp
		msg.timestamp.sec = (int32_t)(epoch_ms / 1000);
		msg.timestamp.nanosec = (uint32_t)(epoch_ns % 1000000000);
		msg.data.capacity = PROCESSED_BUFF_SIZE;
		
		if (xAudioQueue != NULL) {
			if (xQueueReceive(xAudioQueue, &processed_data, 0) == pdPASS) {
				msg.data.data = (uint8_t *)pvPortMalloc(sizeof(uint8_t) * msg.data.capacity);
				msg.data.size = processed_data->size;
				printf("size: %d\n", processed_data->size);

				// for (size_t i=0; i < processed_data->size; i++) {
				// 	msg.data.data[i] = processed_data->data[i];
				// }
				memcpy(msg.data.data, processed_data->data, processed_data->size);
				// printf("Data: %u, %u\n", msg.data.data[0], msg.data.data[1]);
				printf("Publishing: %s\n", msg.format.data);
				RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL));
				vPortFree(processed_data); // Free the queue after use
				vPortFree(msg.data.data);
				
			} else {
				printf("No processed audio data available in the queue\n");
			}

		} else {
			printf("Audio queue is not initialized\n");
		}
	}
}

void micro_ros_task(void * arg)
{
	rcl_allocator_t allocator = rcl_get_default_allocator();
	rclc_support_t support;

	rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();
	RCCHECK(rcl_init_options_init(&init_options, allocator));

#ifdef CONFIG_MICRO_ROS_ESP_XRCE_DDS_MIDDLEWARE
	rmw_init_options_t* rmw_options = rcl_init_options_get_rmw_init_options(&init_options);

	// Static Agent IP and port can be used instead of autodisvery.
	RCCHECK(rmw_uros_options_set_udp_address(CONFIG_MICRO_ROS_AGENT_IP, CONFIG_MICRO_ROS_AGENT_PORT, rmw_options));
	//RCCHECK(rmw_uros_discover_agent(rmw_options));
#endif

	// create init_options
	RCCHECK(rclc_support_init_with_options(&support, 0, NULL, &init_options, &allocator));

	// create node
	rcl_node_t node;
	RCCHECK(rclc_node_init_default(&node, "esp32_raw_audio_node", "", &support));

	// create publisher
	RCCHECK(rclc_publisher_init_default(
		&publisher,
		&node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(foxglove_msgs, msg, RawAudio),
		"raw_audio"));

	// create timer,
	rcl_timer_t timer;
	const unsigned int timer_timeout = 100;
	RCCHECK(rclc_timer_init_default2(
		&timer,
		&support,
		RCL_MS_TO_NS(timer_timeout),
		timer_callback,
		true));

	// create executor
	rclc_executor_t executor;
	RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
	RCCHECK(rclc_executor_add_timer(&executor, &timer));
	
	msg.format.capacity = 100;
	msg.format.data = (char*) malloc(msg.format.capacity * sizeof(char));
	msg.format.size = 0;

	strcpy(msg.format.data, "pcm-s16");
	msg.format.size = strlen("pcm-s16");
	msg.sample_rate = (uint32_t)16000;
	msg.number_of_channels = (uint32_t)1;

	isQueueReady = true;
	while(1){
		rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
		usleep(10000);
	}

	// free resources
	RCCHECK(rcl_publisher_fini(&publisher, &node));
	RCCHECK(rcl_node_fini(&node));
	isQueueReady = false;
  	vTaskDelete(NULL);
}

void app_main(void)
{
#if defined(CONFIG_MICRO_ROS_ESP_NETIF_WLAN) || defined(CONFIG_MICRO_ROS_ESP_NETIF_ENET)
    ESP_ERROR_CHECK(uros_network_interface_initialize());
#endif

#if EXAMPLE_I2S_DUPLEX_MODE
    i2s_example_init_std_duplex();
#else
    i2s_example_init_std_simplex();
#endif
	xAudioQueue = xQueueCreate(20, sizeof(processed_audio_t*)); // Create a queue to hold processed audio data
    assert(xAudioQueue); // Check if queue creation success
	isQueueReady = false;
	//pin micro-ros task in APP_CPU to make PRO_CPU to deal with wifi:
    xTaskCreate(micro_ros_task,
            "uros_task",
            CONFIG_MICRO_ROS_APP_STACK,
            NULL,
            CONFIG_MICRO_ROS_APP_TASK_PRIO,
            NULL);


    /* Step 3: Create writing and reading task, enable and start the channels */
    xTaskCreate(i2s_example_read_task, 
						"i2s_example_read_task", 
						4096, NULL, 5, NULL);
}
