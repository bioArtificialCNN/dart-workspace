#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.hpp>
#include <opencv2/opencv.hpp>
#include <thread>

#include "DHVideoCapture.h"

using namespace std::chrono_literals;

class NodeCamera : public rclcpp::Node
{
private:
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_test_;
    rclcpp::TimerBase::SharedPtr timer_;

    DHVideoCapture _videoCapture;

    void initCamera(int exposure_time) // 1-3ms 0=>auto
    {
        RCLCPP_INFO(this->get_logger(), "Initializing camera...");
        if (!_videoCapture.open(0, 2))
        {
            RCLCPP_ERROR(this->get_logger(), "!!!!!!!!!!!!!!!!!!!!!no camera!!!!!!!!!!!!!!!!!!!!!!!!!");
            rclcpp::shutdown();
        }
        _videoCapture.setExposureTime(exposure_time);
        _videoCapture.setVideoFormat(1280, 1024, true);
        _videoCapture.setFPS(30.0);
        _videoCapture.setBalanceRatio(1.6, 1.3, 2.0, true);
        _videoCapture.setGain(1);
        _videoCapture.setGamma(1);

        _videoCapture.startStream();
        _videoCapture.closeStream();

        _videoCapture.startStream();
        RCLCPP_INFO(this->get_logger(), "Camera initialized.");
    }

    void camera_capture_thread()
    {
        // 从相机读取图像
        int width = 1280;
        int height = 1024;
        cv::Mat image(height, width, CV_8UC3);
        while (rclcpp::ok())
        {
            _videoCapture.read(image);

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    if ((x / 10 + y / 10) % 2 == 0)
                    {
                        image.at<uchar>(y, x) = 255; // 白色
                    }
                    else
                    {
                        image.at<uchar>(y, x) = 0; // 黑色
                    }
                }
            }

            // 将OpenCV图像转换为ROS图像消息
            std_msgs::msg::Header header;
            header.stamp = this->now();
            header.frame_id = "camera";

            sensor_msgs::msg::Image::SharedPtr msg = cv_bridge::CvImage(header, "mono8", image).toImageMsg();
            publisher_test_->publish(*msg);
        }
    }

public:
    NodeCamera() : Node("camera")
    {
        publisher_test_ = this->create_publisher<sensor_msgs::msg::Image>("test/image", 10);
        initCamera(0);
        std::thread(std::bind(&NodeCamera::camera_capture_thread, this)).detach();
    }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<NodeCamera>());
    rclcpp::shutdown();
    return 0;
}