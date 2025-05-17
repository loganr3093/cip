#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <curl/curl.h>

// ANSI colors
#define RESET   "\033[0m"
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN    "\033[1;36m"
#define WHITE   "\033[1;37m"

// buffer for curl response
struct ipbuf {
    char *data;
    size_t len;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct ipbuf *buf = (struct ipbuf *)userp;
    char *ptr = realloc(buf->data, buf->len + realsize + 1);
    if (!ptr) {
        return 0;
    }
    buf->data = ptr;
    memcpy(&(buf->data[buf->len]), contents, realsize);
    buf->len += realsize;
    buf->data[buf->len] = '\0';
    return realsize;
}

int get_public_ip(const char *url, char **out) {
    CURL *curl = curl_easy_init();
    if (!curl) return 0;
    struct ipbuf buf;
    buf.data = malloc(1);
    buf.len = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buf);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "myip-app/1.0");
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, RED "Failed to fetch %s: %s" RESET "\n", url, curl_easy_strerror(res));
        free(buf.data);
        return 0;
    }
    // strip newline
    char *nl = strchr(buf.data, '\n');
    if (nl) *nl = '\0';
    *out = buf.data;
    return 1;
}

void print_local_ips() {
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }
    printf(WHITE "\n== Local IP Addresses ==\n" RESET);
    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        int family = ifa->ifa_addr->sa_family;
        char ip[INET6_ADDRSTRLEN] = {0};
        const char *tag, *color;
        if (family == AF_INET) {
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip));
            if (strncmp(ip, "127.", 4) == 0) {
                tag = "[Loopback IPv4]";
                color = YELLOW;
            } else {
                tag = "[Private IPv4] ";
                color = GREEN;
            }
        } else if (family == AF_INET6) {
            struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)ifa->ifa_addr;
            inet_ntop(AF_INET6, &sa6->sin6_addr, ip, sizeof(ip));
            char *ipstr = ip;
            if (strcmp(ipstr, "::1") == 0) {
                tag = "[Loopback IPv6]";
                color = YELLOW;
            } else if (strncmp(ipstr, "fd", 2) == 0 || strncmp(ipstr, "FC", 2) == 0) {
                tag = "[ULA IPv6]      ";
                color = GREEN;
            } else if (strncmp(ipstr, "fe80", 4) == 0) {
                continue; // skip link-local
            } else {
                tag = "[Global IPv6]   ";
                color = BLUE;
            }
        } else continue;
        printf("  %s%-18s %-42s %s(%s)%s\n", color, tag, ip, RESET, ifa->ifa_name, RESET);
    }
    freeifaddrs(ifaddr);
}

int main() {
    print_local_ips();
    printf(WHITE "\n== Public IP Addresses ==\n" RESET);

    char *pub4 = NULL, *pub6 = NULL;
    if (get_public_ip("https://ipv4.icanhazip.com", &pub4)) {
        printf(CYAN "  %-18s %-42s" RESET "\n", "[Public IPv4]    ", pub4);
        free(pub4);
    }
    if (get_public_ip("https://ipv6.icanhazip.com", &pub6)) {
        printf(CYAN "  %-18s %-42s" RESET "\n", "[Public IPv6]    ", pub6);
        free(pub6);
    }
    return 0;
}
