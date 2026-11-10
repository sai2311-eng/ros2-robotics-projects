# ROS 2 Learning Notes

Personal notes from working through ROS 2 Jazzy step by step (WSL2 Ubuntu 24.04).

## Lesson 1: Nodes

### What a node is
- A node is one running program with one job and a name on the ROS 2 network.
- Program = the file on disk. Node = a live, named instance of it. One program can run as many nodes.
- A node exists on the network only while its process is alive (`ros2 node list` shows it while running, empty after Ctrl+C).
- Splitting a robot into nodes gives reuse (swap one part), isolation (one crash does not kill the rest) and distribution (nodes can run on different computers).

### Commands
```bash
ros2 run <package> <executable>       # start a node
ros2 node list                        # nodes currently on the network
ros2 node info /<node>                # its publishers, subscribers, services, actions
```

### Name and namespace
- Rename a node at launch without changing code: `--ros-args -r __node:=talker2`
- Give it a namespace (like a folder): `--ros-args -r __ns:=/robot1`
- Full name is `<namespace>/<name>`, for example `/robot1/talker`.
- Two nodes with the same full name cause a duplicate warning. Different namespaces avoid that.
- A namespace is also added in front of the node's topics: `/chatter` became `/robot1/chatter`. Use namespaces to separate robots.
- Renaming the node alone does not rename its topics.

### Writing a node (Python, rclpy)
```python
import rclpy
from rclpy.node import Node


class MyFirstNode(Node):
    def __init__(self):
        super().__init__('my_first_node')
        self.count = 0
        self.create_timer(0.2, self.on_timer)     # 5 ticks per second
        self.get_logger().info('Hello, I am alive')

    def on_timer(self):
        self.count += 1
        self.get_logger().info(f'tick {self.count}')


def main():
    rclpy.init()
    node = MyFirstNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


main()
```
- `rclpy.init()` connects the program to ROS 2.
- `spin` is the loop that keeps the node alive and runs callbacks. Without it the program exits right after `__init__` and the node disappears.
- A callback is a function you write that spin runs when something happens. A timer is the simplest source of events.
- A Python script is not reloaded while it runs: Ctrl+C and start it again after every edit.
- Log levels: info (plain), warn (orange), error (red).

### What `ros2 node info` shows
- Sections: subscribers, publishers, service servers, service clients, action servers, action clients. A node only has the ones its code creates.
- A timer does not appear, because it is internal and does not connect to other nodes.
- Every node automatically gets publishers `/parameter_events` and `/rosout`, and 7 parameter service servers. These let tools read and change its parameters.

### Recap
```
program (file) --run--> node (live, named)
                         |- name + namespace
                         |- spin loop
                         |- timer callbacks
                         |- logger (info / warn / error)
                         '- interfaces (pubs, subs, services, actions)
```

## Lesson 2: Topics and messages

### Topics
- A topic is a named channel. Publishers put messages on it, subscribers read from it. Many of each can share one topic.
- A publisher does not know who is listening and does not change if nobody is. Flow is one way, no reply.
- A subscriber survives its publisher restarting: discovery reconnects them by itself. A missed message is gone, and a restarted publisher starts its own variables again (a counter restarts at 0).
- ROS 2 connects a publisher and a subscriber when the **topic name** and the **message type** both match. Nothing is wired by hand: each node declares what it offers or wants in code (`create_publisher`, `create_subscription`).
- `ros2 node info` shows only that node's own interfaces. To see who is connected, use `ros2 topic info /name -v` or `rqt_graph`.

### Relative and absolute names
| Name in code | Kind | With `-r __ns:=/team` |
|---|---|---|
| `'hello'` | relative | `/team/hello` (namespace added) |
| `'/robot1/hello'` | absolute | `/robot1/hello` (namespace ignored) |
- The namespace always applies to the node name. It only applies to a topic name when that is relative.
- Use relative names for things that exist once per robot or arm. Use absolute names for things shared by everything (`/tf`, `/clock`).
- Remap a topic at launch: `--ros-args -r chatter:=news`.

### rqt_graph
- Ovals are nodes, rectangles are topics, arrows show message direction. Hover to highlight connections.
- Press Refresh after starting or stopping nodes. Use "Nodes/Topics (all)" while learning.
- A subscriber with no matching publisher shows up disconnected, which is how typos in topic names show.

### Python publisher and subscriber
```python
# publisher
self.pub = self.create_publisher(String, 'hello', 10)
self.pub.publish(msg)

# subscriber
self.sub = self.create_subscription(String, 'hello', self.on_message, 10)
def on_message(self, msg):
    self.get_logger().info(msg.data)
```
- `create_publisher(type, topic, depth)` and `create_subscription(type, topic, callback, depth)`.
- The callback is run by `spin`. Without `spin`, nothing is ever received.
- Run a loose script with `python3 file.py`. `ros2 run` only works for executables inside an installed package.

### Messages
- A message type is a named structure of fields, and a topic carries exactly one type.
- Write the full name `package/msg/Type`, case-sensitive: `std_msgs/msg/Int32`, `sensor_msgs/msg/LaserScan`. Use Tab completion.
- `ros2 interface show <type>` prints the fields. Messages can nest (`Twist` contains two `Vector3`).
- `Twist`: `linear.x` is forward speed (m/s), `angular.z` is turning speed (rad/s). `/cmd_vel` means "command velocity". Circle radius = v / w.
- `LaserScan`: `angle_min`, `angle_max`, `angle_increment`, `range_min`, `range_max`, `ranges` (distances in metres), `intensities` (return strength), and a `header` with `stamp` and `frame_id`.
- `std_msgs/Int32` is marked deprecated in a comment, but it works.

### Silent failures and the debugging checklist
A different topic name (`hello` vs `hellp`) and a different type (`Int32` vs `String`) both leave the nodes running with no error and no data.
```
1. ros2 topic list                -> do both names exist, spelled the same?
2. ros2 topic info /name -v       -> publisher AND subscriber count above 0?
3. compare the type column        -> same type on both ends?
4. check namespaces and remaps    -> relative vs absolute, __ns, -r
5. ros2 topic echo /name          -> does anything arrive?
```

### Many publishers on one topic
- ROS 2 does not arbitrate. Two nodes publishing on `/cmd_vel` both reach the robot, and it follows whichever message came last.
- A leftover node can hide your change (a new `-0.4` did nothing while an old `+0.4` driver still ran). Check the publisher count in `topic info -v`.
- The robot keeps its last command. Stopping: `ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist "{}"`. A single stop message at program exit may not leave the process in time.

### Pipeline for the diffbot
```
circle_driver --> /cmd_vel (ROS 2) --> ros_gz_bridge --> /cmd_vel (Gazebo) --> DiffDrive plugin
```
`ros_gz_bridge` is a ROS 2 node that copies messages between ROS 2 and Gazebo.

## Lesson 2: QoS (Quality of Service)

QoS is a set of delivery rules that each publisher and subscriber declares. The `10` in `create_publisher(String, 'hello', 10)` is a shortcut for `QoSProfile(depth=10)` with the defaults (reliable, volatile).

| Setting | Options | Meaning |
|---|---|---|
| Reliability | `reliable` / `best_effort` | resend lost messages, or send once and move on |
| Durability | `volatile` / `transient_local` | only new subscribers, or the publisher also keeps stored messages for late joiners |
| History depth | a number | how many messages are kept, so also how many a late joiner can get |

### Compatibility rule
The subscriber must not ask for more than the publisher offers.

| Publisher offers | Subscriber asks | Connects? |
|---|---|---|
| reliable | reliable | yes |
| reliable | best_effort | yes (publisher offers more than needed) |
| best_effort | best_effort | yes |
| best_effort | reliable | no |
| volatile | transient_local | no |

An incompatible pair is not an error: the nodes keep running and no data flows. Echo may print a warning about incompatible QoS. This is the third silent failure, after a wrong topic name and a wrong type. Check the QoS columns in `ros2 topic info /name -v`.

### Which QoS for which data
| Data | Typical QoS | Why |
|---|---|---|
| lidar, camera, IMU | best_effort, volatile, small depth | the newest sample wins, a lost one is fine |
| `/cmd_vel` and other commands | reliable, volatile | a command (above all a stop) must not be lost |
| `/robot_description`, `/map` | reliable, transient_local | published once, late nodes still need it |

A deep queue on a slow subscriber means it processes old frames, so cameras use a small depth.

### Experiments
```bash
# reliability mismatch: publisher best_effort, subscriber reliable -> nothing arrives
ros2 topic pub -r 5 /qos_test std_msgs/msg/String "{data: hi}" --qos-reliability best_effort
ros2 topic echo /qos_test --qos-reliability reliable

# durability: slow publisher (one message per 10 s), late subscriber
ros2 topic pub -r 0.1 /late_test std_msgs/msg/String "{data: latest value}" --qos-durability transient_local --qos-reliability reliable
ros2 topic echo /late_test --qos-durability volatile --qos-reliability reliable         # waits for the next message
ros2 topic echo /late_test --qos-durability transient_local --qos-reliability reliable  # gets the stored one at once
```
Writing the profile out in code:
```python
from rclpy.qos import QoSProfile, ReliabilityPolicy, DurabilityPolicy
qos = QoSProfile(
    depth=3,
    reliability=ReliabilityPolicy.RELIABLE,
    durability=DurabilityPolicy.TRANSIENT_LOCAL,
)
self.pub = self.create_publisher(String, 'hello', qos)
```
With depth 3 a late subscriber got the last 3 messages, with depth 1 only the last one.

## Lesson 3: Services

```
Topic:    publisher ---> subscriber              (continuous, one way, no reply)
Service:  client --request--> server
                 <--response--                   (one question, one answer)
```
- A service is for a one-off question or action ("reset the counter"), not for streaming data.
- A service type has two parts separated by `---`: request above, response below. `std_srvs/srv/Trigger` has an empty request and the response `bool success`, `string message`.
- A topic publisher never waits for anyone. A service client waits for the server, so a call with no server just waits until one appears.

### Commands
```bash
ros2 service list -t                                   # services with their types
ros2 service type /reset_counter                       # type of one service
ros2 interface show std_srvs/srv/Trigger               # request and response fields
ros2 service call /reset_counter std_srvs/srv/Trigger "{}"
ros2 service call /add_two_ints example_interfaces/srv/AddTwoInts "{a: 3, b: 4}"
```
- `ros2 service call <name> <type> "<request as YAML>"` sends one request and prints the response.
- Unfilled request fields default to 0 (`{a: 3}` gives 3 + 0).
- The package is `example_interfaces` (plural). One missing letter gives `Unknown package`.

### Server (Python)
```python
self.srv = self.create_service(AddTwoInts, 'add_two_ints', self.on_request)

def on_request(self, request, response):
    response.sum = request.a + request.b
    return response            # forgetting this leaves the client waiting forever
```
- The callback gets the request and an empty response to fill, and must return the response.
- A missing `return` does not crash anything: the server logs an error and the client waits forever.

### Client (Python)
```python
self.client = self.create_client(AddTwoInts, 'add_two_ints')
while not self.client.wait_for_service(timeout_sec=1.0):
    ...                                    # wait politely for the server
request = AddTwoInts.Request()
request.a = int(sys.argv[1])
future = self.client.call_async(request)
rclpy.spin_until_future_complete(node, future)
result = future.result().sum
```
- `wait_for_service` checks that the server exists. `call_async` sends the request and returns a future, a placeholder for the answer.
- A node only receives the reply while it is spinning, so the client spins until the future is done. Blocking inside a call without spinning would never process the reply.
- `sys.argv` holds the command-line words. A missing one gives a Python `IndexError` from your code, not from ROS.

## Next
Actions and parameters.
