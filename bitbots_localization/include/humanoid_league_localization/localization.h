//
// Created by judith on 08.03.19.
//

#ifndef HUMANOID_LEAGUE_LOCALIZATION_LOCALIZATION_H
#define HUMANOID_LEAGUE_LOCALIZATION_LOCALIZATION_H


#include <vector>
#include <memory>
#include <iterator>

#include <ros/ros.h>
#include <std_srvs/Trigger.h>
#include <dynamic_reconfigure/server.h>

#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>
#include <geometry_msgs/Point.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/PoseArray.h>
#include <geometry_msgs/PoseWithCovariance.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <geometry_msgs/TransformStamped.h>
#include <geometry_msgs/Twist.h>
#include <sensor_msgs/CameraInfo.h>
#include <sensor_msgs/CameraInfo.h>

#include <tf2_ros/transform_listener.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf_conversions/tf_kdl.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Transform.h>
#include <tf2/convert.h>
#include <tf2/utils.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2_ros/buffer.h>
#include <tf2_ros/message_filter.h>
#include <message_filters/subscriber.h>

#include <particle_filter/ParticleFilter.h>
#include <particle_filter/gaussian_mixture_model.h>
#include <particle_filter/CRandomNumberGenerator.h>

#include <humanoid_league_msgs/LineInformationRelative.h>
#include <humanoid_league_msgs/LineSegmentRelative.h>
#include <humanoid_league_msgs/GoalRelative.h>
#include <humanoid_league_msgs/PixelRelative.h>
#include <humanoid_league_msgs/PixelsRelative.h>
#include <humanoid_league_msgs/FieldBoundaryRelative.h>
#include <humanoid_league_msgs/FieldBoundaryInImage.h>

#include <humanoid_league_localization/map.h>
#include <humanoid_league_localization/LocalizationConfig.h>
#include <humanoid_league_localization/ObservationModel.h>
#include <humanoid_league_localization/MotionModel.h>
#include <humanoid_league_localization/StateDistribution.h>
#include <humanoid_league_localization/Resampling.h>
#include <humanoid_league_localization/RobotState.h>

#include <humanoid_league_localization/reset_filter.h>
#include <humanoid_league_localization/Evaluation.h>


#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>
#include <sensor_msgs/image_encodings.h>


namespace hll = humanoid_league_localization;
namespace hlm = humanoid_league_msgs;
namespace gm = geometry_msgs;
namespace pf = particle_filter;


class Localization {

public:
    Localization();

    bool reset_filter_callback(hll::reset_filter::Request &req,
                               hll::reset_filter::Response &res);

    void dynamic_reconfigure_callback(hll::LocalizationConfig &config, uint32_t config_level);

    void LineCallback(const hlm::LineInformationRelative &msg);

    void NonLineCallback(const hlm::LineInformationRelative &msg);

    void GoalCallback(const hlm::GoalRelative &msg);

    void FieldboundaryCallback(const hlm::FieldBoundaryRelative &msg);

    void CornerCallback(const hlm::PixelsRelative &msg);

    void TCrossingsCallback(const hlm::PixelsRelative &msg);

    void CrossesCallback(const hlm::PixelsRelative &msg);

    void CamInfoCallback(const sensor_msgs::CameraInfo &msg);

    void FieldBoundaryInImageCallback(const hlm::FieldBoundaryInImage &msg);


    void init();

    void reset_filter(int distribution);

    void reset_filter(int distribution, double x, double y);

    ros::NodeHandle nh_;


private:
    ros::Subscriber line_subscriber_;
    ros::Subscriber non_line_subscriber_;
    ros::Subscriber goal_subscriber_;
    ros::Subscriber fieldboundary_subscriber_;
    ros::Subscriber corners_subscriber_;
    ros::Subscriber t_crossings_subscriber_;
    ros::Subscriber crosses_subscriber_;
    ros::Subscriber cam_info_subscriber_;
    ros::Subscriber fieldboundary_in_image_subscriber_;


    ros::Publisher pose_publisher_;
    ros::Publisher pose_with_covariance_publisher_;
    ros::Publisher pose_particles_publisher_;
    ros::Publisher lines_publisher_;
    ros::Publisher line_ratings_publisher_;
    ros::Publisher goal_ratings_publisher_;
    ros::Publisher fieldboundary_ratings_publisher_;
    ros::Publisher corner_ratings_publisher_;
    ros::Publisher t_crossings_ratings_publisher_;
    ros::Publisher crosses_ratings_publisher_;
    ros::Publisher non_line_ratings_publisher_;

    ros::ServiceServer service_;
    ros::Timer publishing_timer_;
    tf2_ros::Buffer tfBuffer;
    tf2_ros::TransformListener tfListener;
    tf2_ros::TransformBroadcaster br;


    std::shared_ptr<pf::ImportanceResampling<RobotState>> resampling_;
    std::shared_ptr<RobotPoseObservationModel> robot_pose_observation_model_;
    std::shared_ptr<RobotMotionModel> robot_motion_model_;
    //std::shared_ptr<RobotStateDistribution> robot_state_distribution_;
    std::shared_ptr<RobotStateDistributionStartLeft> robot_state_distribution_start_left_;
    std::shared_ptr<RobotStateDistributionStartRight> robot_state_distribution_start_right_;
    std::shared_ptr<RobotStateDistributionRightHalf> robot_state_distribution_right_half_;
    std::shared_ptr<RobotStateDistributionLeftHalf> robot_state_distribution_left_half_;
    std::shared_ptr<RobotStateDistributionPosition> robot_state_distribution_position_;
    std::shared_ptr<RobotStateDistributionPose> robot_state_distribution_pose_;
    std::shared_ptr<particle_filter::ParticleFilter<RobotState>> robot_pf_;
    RobotState best_estimate_;
    RobotState best_estimate_5_;
    RobotState best_estimate_10_;
    RobotState best_estimate_20_;
    RobotState best_estimate_mean_;

    int resampled = 0;

    hlm::LineInformationRelative line_information_relative_;
    hlm::LineInformationRelative non_line_information_relative_;
    hlm::GoalRelative goal_relative_;
    hlm::FieldBoundaryRelative fieldboundary_relative_;
    hlm::PixelsRelative corners_;
    hlm::PixelsRelative t_crossings_;
    hlm::PixelsRelative crosses_;
    sensor_msgs::CameraInfo cam_info_;
    std::vector<hlm::FieldBoundaryInImage> fieldboundary_in_image_;

    ros::Time last_stamp_lines = ros::Time(0);
    ros::Time last_stamp_goals = ros::Time(0);
    ros::Time last_stamp_fb_points = ros::Time(0);
    ros::Time last_stamp_corners = ros::Time(0);
    ros::Time last_stamp_tcrossings = ros::Time(0);
    ros::Time last_stamp_crosses = ros::Time(0);

    std::vector<gm::Point> interpolateFieldboundaryPoints(gm::Point point1, gm::Point point2);

    void publishing_timer_callback(const ros::TimerEvent &e);

    std::shared_ptr<Map> lines_;
    std::shared_ptr<Map> goals_;
    std::shared_ptr<Map> field_boundary_;
    std::shared_ptr<Map> corner_;
    std::shared_ptr<Map> t_crossings_map_;
    std::shared_ptr<Map> crosses_map_;


    gmms::GaussianMixtureModel pose_gmm_;
    std::vector<gm::Point> line_points_;
    std::vector<gm::Point> non_line_points_;
    particle_filter::CRandomNumberGenerator random_number_generator_;
    hll::LocalizationConfig config_;
    std_msgs::ColorRGBA marker_color;
    bool valid_configuration_ = false;

    void publish_pose();

    void publish_pose_with_covariance();

    void publish_lines();

    void publish_line_ratings();

    void publish_goal_ratings();

    void publish_field_boundary_ratings();

    void publish_corner_ratings();

    void publish_t_crossings_ratings();

    void publish_crosses_ratings();

    void publish_non_line_ratings();

    void publish_lines_map();

    void publish_map();

    geometry_msgs::TransformStamped transformOdomBaseLink;
    bool initialization = true;

    void getMotion();

    geometry_msgs::Vector3 movement_;
    geometry_msgs::Vector3 movement2_;
    bool new_linepoints_ = false;
    bool robot_moved = false;
    int timer_callback_count = 0;
};


#endif //HUMANOID_LEAGUE_LOCALIZATION_LOCALIZATION_H
