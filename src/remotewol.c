#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>


const char* EXIT_CMD = "exit";
const char* CONFIG_PATH = "/etc/config/remotewol";

/**
 * device node
 */
struct lan_device {
    char *ip;
    char *mac;
    char *command;
};

void cleanup();
int isWake(const char* ip);
void wakeup(char* ip, const char* mac);
int load_config_port();
struct lan_device* get_device_by_command(char *command);
char* get_boardcastip_from_device_ip(char *ip);
int send_magic_pack(const char* mac, const char* ip);

static int sockfd = -1;

int main() {
    struct sockaddr_in6 server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buf[256];
    int port = load_config_port();
    if (port <= 0)
    {
        puts("Port is invalid, Program will exit");
        return -1;
    }

    printf("Try to listen port on: %d\n", port);

    atexit(cleanup);

    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port);
    server_addr.sin6_addr = in6addr_any;

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        memset(buf, 0, sizeof(buf));
        puts("Waiting message...");
        ssize_t recv_len = recvfrom(sockfd, buf, sizeof(buf) - 1, 0, 
                                   (struct sockaddr*)&client_addr, &client_len);
        if (recv_len < 0) {
            perror("recvfrom failed");
            continue;
        }
        buf[recv_len] = '\0';  // 确保字符串结束

        printf("Message received from client: %s\n", buf);

        if (strncmp(buf, EXIT_CMD, 4) == 0)
        {
            puts("Exiting...");
            close(sockfd);
            break;
        }

        struct lan_device *device = get_device_by_command(buf);
        if (device == NULL)
        {
            puts("Device not found");
            continue;
        }
        printf("Executing command: %s\n", device->command);
        wakeup (device->ip, device->mac);

        // 释放设备结构体的所有成员
        free(device->ip);
        free(device->mac);
        free(device->command);
        free(device);
    }
    return 0;
}

/// <summary>
/// �ж�Ŀ�������Ƿ�����
/// </summary>
/// <param name="ip"></param>
/// <returns>1-����, 0-����</returns>
int isWake(const char* ip)
{
    char command[100];
    //ʹ��ping������Ŀ�������Ƿ����, -c 1 ֻpingһ��, -W 1 ��ʱʱ��1��, gerp����ping�����100% packet loss�ַ�, -c ֻ��ʾ�ҵ����к�
    snprintf(command, sizeof(command), "ping -c 1 -W 1 %s | grep -c \"100%% packet loss\"", ip);
    FILE* file = popen(command, "r");
    char cmd_value = getc(file);
    pclose(file);
    return cmd_value == '0';
}

/// <summary>
/// ����PC
/// </summary>
/// <param name="ip"></param>
/// <param name="mac"></param>
void wakeup(char* ip, const char* mac)
{
    if (isWake(ip))
    {
        printf("%s has already online\n", ip);
        return;
    }
    char *boardcastip = get_boardcastip_from_device_ip(ip);
    int ret = send_magic_pack(mac, boardcastip);
    if (ret == 0)
        puts("magic pack has been sended");
    free(boardcastip);
}


/// <summary>
/// UDP��ʽ����ħ����
/// </summary>
/// <param name="mac">������������ַ, ��ʽ XX:XX:XX:XX:XX:XX</param>
/// <param name="ip">�������ڵ�IP��ַ, ��ʽ XXX.XXX.XXX.XXX</param>
/// <returns></returns>
int send_magic_pack(const char* mac, const char* ip)
{
    //if (mac == NULL || strlen(mac) == 0) { //���mac��ַΪ�գ����ش���
    //    printf("wol failed, because mac is null");
    //    return -1;
    //}
    //if (strlen(mac) != 17) { //���mac��ַ���������ĵ�ַ�����ش���
    //    printf("wol failed, because mac is %s\n", mac);
    //    return -1;
    //}

    int ret = -1;
    int send_length = -1;
    unsigned char packet[102] = { 0 };
    struct sockaddr_in addr;
    int sockfd, i, j, option_value = 1;
    unsigned char mactohex[6] = { 0 };

    sscanf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
        (unsigned int*)&mactohex[0],
        (unsigned int*)&mactohex[1],
        (unsigned int*)&mactohex[2],
        (unsigned int*)&mactohex[3],
        (unsigned int*)&mactohex[4],
        (unsigned int*)&mactohex[5]);
    printf("Mac is %s,mac to hex is %02x%02x%02x%02x%02x%02x\n", mac, mactohex[0], mactohex[1], mactohex[2], mactohex[3], mactohex[4], mactohex[5]);
    //Mac is E4:54:E8:A7:E0:57,mac to hex is e454e8a7e057
    // 
    //����magic packet
    for (i = 0; i < 6; i++) { //6�ԡ�FF��ǰ׺
        packet[i] = 0xFF;
    }

    for (i = 1; i < 17; i++) { //Ŀ��������MAC��ַ���ظ�16��
        for (j = 0; j < 6; j++) {
            packet[i * 6 + j] = mactohex[j];
        }
    }

    //UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    //�㲥
    ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &option_value, sizeof(option_value));
    if (ret < 0) {
        //printf("set socket opt failed, errno=%d\n", errno);
        close(sockfd);
        return ret;
    }

    memset((void*)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9);
    addr.sin_addr.s_addr = inet_addr(ip);//UDP�㲥��ַ

    //���͹㲥
    send_length = sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr));
    //if (send_length < 0) {
        //printf("wol send data ret <= 0, ret = %d, errno = %d", send_length, errno);
    //}

    close(sockfd);
    return ret;
}

int load_config_port()
{
    FILE *fp = fopen(CONFIG_PATH, "r");
    if (!fp) {
        return -1;
    }

    char line[256];
    int port = -1;

    while (fgets(line, sizeof(line), fp)) {
        // 去除行尾的换行符
        line[strcspn(line, "\n")] = 0;
        
        // 查找包含 port 的行
        char *port_str = strstr(line, "option port");
        if (port_str) {
            // 提取数字
            char *value = strchr(port_str, '\'');
            if (value) {
                port = atoi(value + 1);
                break;
            }
        }
    }

    fclose(fp);
    return port;
}

struct lan_device* get_device_by_command(char *command) 
{
    FILE *fp = fopen(CONFIG_PATH, "r");
    if (!fp) return NULL;

    struct lan_device *device = NULL;
    char line[256];
    char current_ip[64] = {0};
    char current_mac[64] = {0};
    char current_command[64] = {0};
    int in_device = 0;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;

        if (strstr(line, "config devices")) {
            in_device = 1;
            memset(current_ip, 0, sizeof(current_ip));
            memset(current_mac, 0, sizeof(current_mac));
            memset(current_command, 0, sizeof(current_command));
            continue;
        }

        if (in_device) {
            char value[256];
            if (sscanf(line, "\toption ip '%[^']'", value) == 1) {
                strncpy(current_ip, value, sizeof(current_ip) - 1);
            }
            else if (sscanf(line, "\toption mac '%[^']'", value) == 1) {
                strncpy(current_mac, value, sizeof(current_mac) - 1);
            }
            else if (sscanf(line, "\toption command '%[^']'", value) == 1) {
                strncpy(current_command, value, sizeof(current_command) - 1);
                
                // 如果找到匹配的命令，创建设备对象并返回
                if (strncmp(current_command, command, strlen(current_command)) == 0) {
                    device = malloc(sizeof(struct lan_device));
                    device->ip = strdup(current_ip);
                    device->mac = strdup(current_mac);
                    device->command = strdup(current_command);
                    break;
                }
            }
        }
        // 遇到空行或其他配置项，重置设备解析状态
        if (strlen(line) == 0 || line[0] != '\t') {
            in_device = 0;
        }
    }
    fclose(fp);
    return device;
}

/**
 * input like 192.168.5.2
 * return 192.168.5.255, replace latest number to 255
 */
char* get_boardcastip_from_device_ip(char *ip) 
{
    char *boardcastip = (char*)malloc(16);
    char *lastdot = strrchr(ip, '.');
    int len = lastdot - ip;
    strncpy(boardcastip, ip, len);
    boardcastip[len] = '\0';
    strcat(boardcastip, ".255");
    return boardcastip;
}

void cleanup() {
    if (sockfd >= 0) {
        close(sockfd);
    }
}