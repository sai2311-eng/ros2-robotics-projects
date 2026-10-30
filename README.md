# ROS 2 Robotics Projects

Simulation-only robotics projects built step by step with **ROS 2 Jazzy** and **Gazebo Harmonic**. No physical hardware is needed: everything runs on a laptop (developed on Windows 11 + WSL2 Ubuntu 24.04).

The goal of the series is to learn the ROS 2 stack by building each piece by hand and understanding why it exists.

## Projects

| # | Project | Status | What it covers |
|---|---------|--------|----------------|
| 0 | [ROS 2 basics in C++](00-ros2-basics/) | Done | Publisher, subscriber, service, custom action server, QoS |
| 1 | [Differential-drive robot in Gazebo](project-1-diffbot/) | Done | URDF/Xacro, TF, Gazebo world and sensors, `ros_gz_bridge`, launch files, RViz2, keyboard driving |
| 2 | SLAM mapping with `slam_toolbox` | Planned | Scan matching, loop closure, saving maps |
| 3 | Autonomous navigation with Nav2 | Planned | Localization, costmaps, planners, recovery behaviors |
| 4 | Autonomous exploration | Planned | Frontier-based exploration on top of SLAM + Nav2 |
| 5 | Warehouse delivery robot | Planned | Waypoint missions, task queue, battery behavior |

## Environment

- Windows 11 with WSL2, Ubuntu 24.04
- ROS 2 Jazzy (`ros-jazzy-desktop`, `ros-jazzy-ros-gz`)
- Gazebo Harmonic (the new Gazebo Sim, not Gazebo Classic)
- Build tool: `colcon`

## Repository layout

```
ros2-robotics-projects/
├── README.md
├── LICENSE
├── 00-ros2-basics/
│   ├── README.md        <- C++ publisher, subscriber, service, action
│   └── robot_basics/    <- the ROS 2 package
└── project-1-diffbot/
    ├── README.md        <- full write-up of project 1
    └── diffbot_sim/     <- the ROS 2 package
```

Each project is a self-contained folder with its own README. A ROS 2 workspace discovers packages by their `package.xml`, so you can clone this repo anywhere under a workspace's `src/` folder and run `colcon build`.

## License

MIT, see [LICENSE](LICENSE).
