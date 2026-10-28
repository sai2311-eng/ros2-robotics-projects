# Project 1: Differential-drive robot in Gazebo

A two-wheeled differential-drive robot written from scratch (URDF/Xacro), simulated in **Gazebo Harmonic**, connected to **ROS 2 Jazzy** through `ros_gz_bridge`, and visualized in **RViz2**. It has a 360-degree lidar and an IMU, and you drive it with the keyboard.

This is the foundation for the later projects (SLAM, Nav2, exploration): they all need exactly this kind of robot, with a correct TF tree, odometry and a lidar scan.

```
 Gazebo Sim (physics, sensors)  <-- ros_gz_bridge -->  ROS 2 nodes
   wheels, lidar, IMU                                    /cmd_vel  (you -> robot)
                                                         /scan /odom /imu /joint_states /tf (robot -> you)
                                                                  |
              robot_state_publisher --> /tf, /robot_description --> RViz2
```

## What was built and verified

- Robot model with chassis, two drive wheels, a caster, a lidar and an IMU, all parameterized with Xacro properties and macros.
- A 10 m x 10 m Gazebo room with a box and a cylinder as obstacles.
- A launch file that starts Gazebo, spawns the robot, bridges 7 topics and opens RViz2.
- Verified in simulation: the robot drives forward, backward and turns from the keyboard; `/odom` changes while driving; `/joint_states` reports wheel angles; the lidar scan is published with `frame_id: lidar_link`; the TF tree is `odom -> base_footprint -> base_link -> ...`.

## Repository layout

```
diffbot_sim/
├── package.xml                  package manifest and run-time dependencies
├── CMakeLists.txt               installs urdf/ launch/ config/ worlds/ into the package share folder
├── urdf/diffbot.urdf.xacro      robot body, sensors, Gazebo plugins
├── worlds/simple_world.sdf      Gazebo world: room, obstacles, simulation systems
├── config/bridge.yaml           Gazebo <-> ROS 2 topic mappings
├── config/diffbot.rviz          saved RViz2 layout
└── launch/
    ├── display.launch.py        robot model in RViz2 with joint sliders (no Gazebo)
    └── sim.launch.py            full simulation: Gazebo + spawn + bridge + RViz2
```

## Requirements

- Ubuntu 24.04 (native or WSL2) with ROS 2 Jazzy: `ros-jazzy-desktop`
- Gazebo Harmonic integration: `ros-jazzy-ros-gz`
- `ros-jazzy-joint-state-publisher-gui` (only for `display.launch.py`)
- `ros-jazzy-teleop-twist-keyboard` (for driving)
- `colcon` (`python3-colcon-common-extensions`)
- On WSL2, GUI windows work through WSLg (Windows 11). Roughly 4 GB RAM is enough for this project; SLAM and Nav2 will need more headroom.

## Build and run

```bash
# 1. Put the package in a workspace and build it
mkdir -p ~/ros2_ws/src
cp -r diffbot_sim ~/ros2_ws/src/          # or clone this repo under ~/ros2_ws/src
cd ~/ros2_ws
colcon build --symlink-install --packages-select diffbot_sim
source install/setup.bash

# 2. Make sure no old Gazebo is running (important, see Troubleshooting)
pgrep -fa "gz sim" || echo clean

# 3. Start the simulation
ros2 launch diffbot_sim sim.launch.py
```

Wait about 40 seconds on a slow machine. The robot appears in Gazebo about 5 seconds after Gazebo starts, and RViz2 opens next to it.

Drive in a second terminal (click inside it so it has keyboard focus):

```bash
source ~/ros2_ws/install/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard
```

| Key | Action | Key | Action |
|-----|--------|-----|--------|
| `i` | forward | `,` | backward |
| `j` | rotate left | `l` | rotate right |
| `u` / `o` | forward + curve left / right | `k` | **stop** |
| `q` / `z` | speeds +10% / -10% | | |

Press `k` to stop: the Gazebo `DiffDrive` plugin keeps the last velocity command until a new one arrives.

To see the model without Gazebo (check the URDF and TF with sliders):

```bash
ros2 launch diffbot_sim display.launch.py
```

### RViz2 for the simulation

The saved layout contains Grid, RobotModel and TF with Fixed Frame `base_link`. For the simulation:

1. Set **Fixed Frame** to `odom` so the robot visibly moves.
2. Add a **LaserScan** display on `/scan` (size about 0.05 m). If nothing appears, set its Reliability Policy to *Best Effort*.

## The robot

Coordinate convention (REP-103): **x forward, y left, z up**.

| Part | Geometry | Mass | Notes |
|------|----------|------|-------|
| Chassis (`base_link`) | box 0.40 x 0.25 x 0.10 m | 5.0 kg | centered 0.03 m above the axle line |
| Wheels | cylinder r = 0.05 m, width 0.04 m | 0.5 kg each | `continuous` joints at y = +/-0.15 m (separation 0.30 m) |
| Caster | sphere r = 0.025 m | 0.1 kg | `fixed`, frictionless, at x = +0.15 m |
| Lidar | cylinder r = 0.04 m, h = 0.05 m | 0.1 kg | on top, x = +0.10 m, z = 0.105 m |
| IMU | no geometry | 0.01 kg | at the chassis center |

### Frames

```
odom                              (published by Gazebo DiffDrive, via the bridge)
 └─ base_footprint                (point on the ground under the robot)
     └─ base_link                 (0, 0, wheel_radius above the ground)
         ├─ left_wheel            (continuous, axis = joint z, rpy = -pi/2 about x)
         ├─ right_wheel
         ├─ caster_link
         ├─ lidar_link
         └─ imu_link
```

- `base_footprint` is an empty link marking the ground point. Navigation uses it as the robot position.
- The wheel joints are rotated by -pi/2 about x so that the joint z axis points along +y. This means both wheels use `axis 0 0 1`, and a positive angle rolls the robot forward.
- Wheel radius and separation are Xacro properties used twice: for the geometry and for the `DiffDrive` plugin. They cannot drift apart.

### URDF concepts used

- **Link**: a rigid body with `<visual>` (looks, used by RViz and rendering), `<collision>` (physics shape) and `<inertial>` (mass and inertia tensor). Without `<inertial>` Gazebo ignores or mishandles the link.
- **Joint**: connects parent and child. Types used: `fixed` and `continuous`.
- **Xacro**: `<xacro:property>` constants, `${...}` math, and macros. The `box_inertia`, `cylinder_inertia` and `sphere_inertia` macros compute inertia from standard formulas; the `wheel` macro is instantiated twice (left, right).

### Gazebo-specific parts (inside the URDF)

`<gazebo>` blocks are ignored by ROS and read by Gazebo when the robot is spawned (the URDF is converted to SDF):

| Block | Purpose |
|-------|---------|
| `mu1`/`mu2` on wheels = 1.0, on caster = 0.0 | wheels grip; the caster ball slides |
| `gpu_lidar` sensor | 360 rays, +/-pi, 10 Hz, range 0.12 to 10 m, topic `scan`, `gz_frame_id` = `lidar_link` |
| `imu` sensor | 100 Hz, topic `imu`, `gz_frame_id` = `imu_link` |
| `DiffDrive` plugin | listens on `cmd_vel`, drives the wheels, publishes `odom` and the `odom -> base_footprint` TF at 50 Hz, accelerations limited to 1.0 m/s^2 and 2.0 rad/s^2 |
| `JointStatePublisher` plugin | publishes all joint angles on `joint_states` |

`gz_frame_id` makes the sensor messages carry the URDF frame names; without it the frame names are long Gazebo paths and RViz cannot place the data in the TF tree. (`gz sdf` prints a harmless warning that `gz_frame_id` is not in the SDF spec; it is copied through and works.)

## The Gazebo world

`worlds/simple_world.sdf` declares the simulation **systems** (plugins). A world without systems is just inert shapes.

| System | Why it is needed |
|--------|------------------|
| `Physics` | steps the simulation (gravity, contacts, joints), 1 ms step |
| `UserCommands` | lets other programs create entities, which is how the robot is spawned |
| `SceneBroadcaster` | sends the scene to the GUI |
| `Sensors` (`ogre2`) | renders the GPU lidar |
| `Imu` | simulates IMU sensors |

Contents: a ground plane, four walls forming a 10 m x 10 m room (10.2 m long so the corners overlap), a 0.5 m box at (2, 1) and a cylinder (r = 0.3 m, h = 0.8 m) at (-1.5, -2). All are `static`. The world is named `simple_world`, and the spawn command must use the same name.

## The bridge

Gazebo and ROS 2 use different message systems; `ros_gz_bridge` translates per topic. `config/bridge.yaml` defines:

| ROS topic | Type | Direction | Source / purpose |
|-----------|------|-----------|------------------|
| `/clock` | `rosgraph_msgs/Clock` | Gazebo -> ROS | simulation time |
| `/cmd_vel` | `geometry_msgs/Twist` | ROS -> Gazebo | drive commands |
| `/odom` | `nav_msgs/Odometry` | Gazebo -> ROS | wheel odometry |
| `/tf` | `tf2_msgs/TFMessage` | Gazebo -> ROS | `odom -> base_footprint` |
| `/joint_states` | `sensor_msgs/JointState` | Gazebo -> ROS | wheel angles |
| `/scan` | `sensor_msgs/LaserScan` | Gazebo -> ROS | lidar |
| `/imu` | `sensor_msgs/Imu` | Gazebo -> ROS | IMU |

The `gz_topic_name` in each entry must equal the `<topic>` set in the URDF plugin or sensor. If they differ, that topic stays silent.

## What `sim.launch.py` does

1. **Gazebo**: includes `ros_gz_sim/gz_sim.launch.py` with `-r <world>` (run immediately).
2. **`robot_state_publisher`**: expands the Xacro, publishes `/robot_description` and the TF for fixed and wheel joints.
3. **Spawn** (after a 5 s timer, so Gazebo is ready): `ros_gz_sim create -world simple_world -name diffbot -topic robot_description -z 0.05`.
4. **Bridge**: `parameter_bridge` with `bridge.yaml`.
5. **RViz2** with the saved layout.

Every node uses `use_sim_time: True`, so they follow the simulator `/clock`. TF is stamped with simulation time; a node on wall-clock time would reject those transforms.

## Verify it works

```bash
ros2 node list                                   # robot_state_publisher, ros_gz_bridge, rviz, ...
ros2 topic hz /scan                              # lidar rate (target 10 Hz; lower if the sim runs slowly)
ros2 topic echo /scan --field header.frame_id --once    # lidar_link
ros2 topic echo /odom --field pose.pose.position --once # changes while driving
ros2 topic echo /joint_states --once             # left_wheel_joint, right_wheel_joint angles
ros2 run tf2_tools view_frames                   # writes frames_*.pdf with the TF tree
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.2}}"   # drive without the keyboard
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.0}}"   # ...and stop
```

## Troubleshooting (problems actually hit while building this)

| Symptom | Cause and fix |
|---------|---------------|
| `file 'display.launch.py' was not found in the share directory` | `CMakeLists.txt` had `DESTINATION share/${diffbot_sim}`. It must be `${PROJECT_NAME}`. The wrong variable expands to nothing, so nothing is installed under `share/diffbot_sim/`. Fix it and rebuild. |
| New launch/config file "not found" after creating it | With `--symlink-install`, edits to existing installed files apply immediately, but **new files need one rebuild** so the link is created. |
| RViz2 opens empty | A fresh RViz has no displays. Set the Fixed Frame, add RobotModel (`/robot_description`) and TF, then **File -> Save Config As** and load it with `-d`. |
| Robot never spawns / appears in the wrong window | A second, older Gazebo instance was still running. Two servers with the same world name share topics and services, so the spawn request is answered by only one. Always run `pkill -f "gz sim"` and check `pgrep -fa "gz sim"` before launching. |
| `/joint_states` or TF missing when inspecting from another terminal | The launch was not running at that moment. The inspection commands only work while the launch is alive. |
| Robot does not respond to keys | The teleop terminal must be focused. Verify the chain with `ros2 topic echo /cmd_vel` and a manual `ros2 topic pub`. |
| Robot keeps driving after releasing a key | `DiffDrive` holds the last command. Press `k`. |
| `/scan` rate below 10 Hz | The sim ran at a real-time factor of about 0.5 to 1.0 on a 4 GB WSL VM with software rendering. Check with `gz topic -e -t /stats -n 1`. |
| No laser points in RViz | Set the LaserScan Reliability Policy to Best Effort and confirm the Fixed Frame exists in TF. |

## Known limitations

- Odometry comes from the Gazebo `DiffDrive` plugin, not `ros2_control`. It is simple but is not the same interface a real robot would use.
- No sensor noise is modeled yet; the lidar and IMU are ideal.
- The saved RViz layout uses Fixed Frame `base_link` and has no LaserScan display (see RViz2 notes above).
- Performance is limited by the development machine; the Gazebo GUI and GPU lidar are CPU-rendered under WSL2.

## What comes next

Project 2 adds `slam_toolbox` on top of this robot: it consumes `/scan` and the `odom` TF, publishes the `map -> odom` transform and the `/map` occupancy grid, and lets us drive around and save a map of the room.
