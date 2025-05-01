# Creating a NAT Instance

## Step 1

Run NATInstanceSetup.yaml cloud formation template

## Step 2 (1 network interface setup)

Manually create NAT Instance with below instructions:

------------------------------------------------------------------------------------------------------------------------------------------------------------------

Step 1: Launch an EC2 Instance

    Navigate to the EC2 Console:
        Go to the AWS Management Console and open the EC2 Dashboard.

    Choose an AMI:
        Select an AMI, such as Amazon Linux 2 or a specific NAT instance-ready AMI (if available in your region).

    Instance Type:
        Choose an instance type that matches your expected network traffic needs. A t2.micro is suitable for small workloads.

    Network Configuration:
        Place the instance in the Public Subnet created in your CloudFormation template.
        Assign a public IP address or attach an Elastic IP (EIP) after launch.

    Security Group:
        Create or select a security group with the following rules:
            Inbound Rules:
                Allow SSH (port 22) for management from trusted IPs.
                Allow all traffic (protocol -1) from the private subnet CIDR range (e.g., 10.0.2.0/24).
            Outbound Rules:
                Allow all outbound traffic (0.0.0.0/0).

    Launch the Instance:
        Review and launch the instance, ensuring you have a key pair for SSH access.

Step 2: Configure the Instance

    SSH into the Instance:
        Use your key pair to SSH into the EC2 instance.

    Enable IP Forwarding:
        Run the following command to enable IP forwarding temporarily:

echo 1 > /proc/sys/net/ipv4/ip_forward

To make it persistent, edit the sysctl configuration:

sudo nano /etc/sysctl.conf

    Add or uncomment the line:

net.ipv4.ip_forward = 1

Save the file and apply the settings:

        sudo sysctl -p

Set Up IPTables Rules:

    Add the following rule to configure NAT:

sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE

Persist the rules using iptables-save:

        sudo yum install iptables-services -y
        sudo service iptables save

Step 3: Disable Source/Destination Check

    In the AWS Console:
        Select the instance.
        Choose Actions → Networking → Change Source/Destination Check.
        Disable source/destination checks.

Step 4: Update the Private Route Table

    Navigate to the Route Tables section of the VPC console.
    For the Private Route Table, add or modify a route:
        Destination: 0.0.0.0/0
        Target: The ID of your NAT EC2 instance.

Step 5: Attach an Elastic IP (Optional)

    If not already done, allocate an Elastic IP in the VPC section and associate it with your NAT instance to ensure a static public IP.

Step 6: Test the Configuration

    Launch an instance in the Private Subnet.
    Test internet access (e.g., by pinging an external IP) to confirm traffic is routed through the NAT instance.

By following these steps, your NAT EC2 instance will act as a NAT gateway for private subnet instances. Let me know if you need further clarifications!

------------------------------------------------------------------------------------------------------------------------------------------------------------------

## Step 2 (2 network interfaces setup)

If using 2 network interfaces with simple forwarding of traffic:

------------------------------------------------------------------------------------------------------------------------------------------------------------------

To set up a NAT instance with two network interfaces (one for the internet and one for communicating with private subnet instances), follow these steps:
Step 1: Create Network Interfaces

    Public Network Interface:
        This will handle internet traffic.
        Attach it to the public subnet and assign a public IP address (or associate an Elastic IP).

    Private Network Interface:
        This will handle traffic from the private subnet.
        Attach it to the private subnet without a public IP address.

Step 2: Launch the NAT EC2 Instance

    AMI Selection:
        Choose an AMI such as Amazon Linux 2 or an official NAT instance AMI (if available).

    Instance Type:
        Select an instance type based on the traffic volume (t2.micro for small workloads).

    Network Configuration:
        Attach both the public and private network interfaces during launch or after the instance has been launched:
            Primary Network Interface: Attach the public network interface.
            Secondary Network Interface: Attach the private network interface.

    Security Group:
        Create a security group with the following rules:
            Inbound Rules:
                Allow traffic from the private subnet on all protocols (to the private network interface).
                Allow SSH from trusted IP addresses (to the public network interface).
            Outbound Rules:
                Allow all outbound traffic (0.0.0.0/0) for internet access (via the public interface).

Step 3: Disable Source/Destination Check

    By default, EC2 instances perform source/destination checks. For NAT instances, these checks must be disabled.
    Steps:
        In the EC2 console, select the NAT instance.
        Go to Actions → Networking → Change Source/Destination Check.
        Disable the check.

Step 4: Configure the NAT Instance

    SSH into the NAT Instance:
        Connect using the key pair.

    Enable IP Forwarding:
        Run the following command to enable IP forwarding temporarily:

echo 1 > /proc/sys/net/ipv4/ip_forward

Make it persistent:

sudo vim /etc/sysctl.conf

Add or uncomment the line:

net.ipv4.ip_forward = 1

Save and apply the changes:

    sudo sysctl -p

Set Up IPTables for Traffic Forwarding:

    Configure NAT and forwarding between the interfaces:

sudo iptables -t nat -A POSTROUTING -o enX0 -j MASQUERADE
sudo iptables -A FORWARD -i enX1 -o enX0 -m state --state RELATED,ESTABLISHED -j ACCEPT
sudo iptables -A FORWARD -i enX0 -o enX1 -j ACCEPT

    Replace eth0 with the public interface and eth1 with the private interface (adjust based on actual interface names in the instance).

Persist the IPTables rules:

        sudo yum install iptables-services -y
        sudo service iptables save

Step 5: Update Private Route Table

    In the VPC console:
        Go to Route Tables.
        Select the Private Route Table.
        Add or update a route:
            Destination: 0.0.0.0/0
            Target: The private IP of the NAT instance on the private network interface.

Step 6: Test the Configuration

    Test from a Private EC2 Instance:
        SSH into the instance in the private subnet (via a bastion host or Session Manager).
        Test connectivity to the internet using commands like ping or curl:

        curl http://example.com

    Debugging:
        If traffic does not forward, ensure:
            Source/destination check is disabled.
            IPTables rules are correct.
            Routes in the private route table point to the NAT instance's private IP.

Summary of Network Interface Setup

    Public Interface:
        Subnet: Public Subnet
        Assign: Public/Elastic IP
        Role: Communicates with the internet.
    Private Interface:
        Subnet: Private Subnet
        Assign: Private IP
        Role: Receives and routes traffic from private subnet instances.

By following these steps, your NAT instance will successfully forward traffic between the private subnet and the internet. Let me know if you need more details!

------------------------------------------------------------------------------------------------------------------------------------------------------------------

## Step 3

Launch instance in private subnet, ensure security rules are as follows:

------------------------------------------------------------------------------------------------------------------------------------------------------------------

Configuring the Security Group for the Private EC2 Instance
Outbound Rules

    Allow All Outbound Traffic:
        Add an outbound rule to allow all traffic (0.0.0.0/0) so the instance can connect to the internet through the NAT instance.
        This is the default setting for most security groups.

    Example Rule:
        Type: All traffic
        Protocol: All
        Port Range: All
        Destination: 0.0.0.0/0

Inbound Rules

    Allow Required Inbound Traffic:
        Define specific inbound rules based on the services running on the private EC2 instance. Examples:
            SSH:
                Type: SSH
                Protocol: TCP
                Port Range: 22
                Source: The private IP range of your VPC (e.g., 10.0.0.0/16) or specific trusted IP addresses for management.
            Application-Specific Ports:
                Add rules for ports required by your application (e.g., HTTP 80 or HTTPS 443 for web servers).

    Example Rule:
        Type: Custom TCP Rule (or HTTP/HTTPS, depending on the app)
        Protocol: TCP
        Port Range: Application-specific port
        Source: Specific CIDR or security group (e.g., the public subnet’s CIDR range or the security group of your NAT instance).

    Deny Unnecessary Inbound Traffic:
        Ensure no broader rules, such as allowing all inbound traffic (0.0.0.0/0), are present unless explicitly required.

------------------------------------------------------------------------------------------------------------------------------------------------------------------

## Step 4

If removing IP forwarding:

sudo vim /etc/sysctl.conf 
->
comment out net.ipv4.ip_forward = 1
->
sudo sysctl -p

Then:

sudo iptables -t nat -D POSTROUTING -o enX0 -j MASQUERADE
sudo iptables -D FORWARD -i enX1 -o enX0 -m state --state RELATED,ESTABLISHED -j ACCEPT
sudo iptables -D FORWARD -i enX0 -o enX1 -j ACCEPT

Verify rules are updated:

sudo iptables -L -t nat -v -n
sudo iptables -L -v -n

## Step 5

in public VM:

sudo apt-get update
sudo apt-get install git
sudo apt-get install build-essential
git clone https://github.com/kohler/click.git

Clone CS686 repo

in private VM:

Clone CS686 repo

## Step 6



### TROUBLESHOOTING

tried to do curling google.com under 3 scenarios using modified sandbox tester
- no forwarding and no click
- with forwarding only
- with click only