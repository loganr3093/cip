// TODO: Add documentation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <curl/curl.h>
#include <getopt.h>

// ANSI colors
#define RESET   "\033[0m"
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define CYAN    "\033[1;36m"
#define WHITE   "\033[1;37m"

// buffer for curl response
struct ipbuf
{
    char *data;
    size_t len;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct ipbuf *buf = (struct ipbuf *)userp;
    char *ptr = realloc(buf->data, buf->len + realsize + 1);
    if (!ptr) return 0;
    buf->data = ptr;
    memcpy(buf->data + buf->len, contents, realsize);
    buf->len += realsize;
    buf->data[buf->len] = '\0';
    return realsize;
}

int get_public_ip(const char *url, char **out)
{
    CURL *curl = curl_easy_init();
    if (!curl) return 0;
    struct ipbuf buf =
    {
        .data = malloc(1),
        .len = 0
    };

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "cip/1.0");
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        fprintf(stderr, RED "Failed: %s\n" RESET, curl_easy_strerror(res));
        free(buf.data);
        return 0;
    }

    char *nl = strchr(buf.data, '\n');
    if (nl)
    {
        *nl = '\0';
    }

    *out = buf.data;
    return 1;
}


void print_local_ips(int show_v4, int show_v6, const char *if_filter)
{
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return;
    }
    printf(WHITE "\n== Local IP Addresses ==\n" RESET);
    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr) continue;
        if (if_filter && strcmp(ifa->ifa_name, if_filter) != 0) continue;

        int fam = ifa->ifa_addr->sa_family;
        char ip[INET6_ADDRSTRLEN] = {0};
        const char *tag, *color;

        if (fam == AF_INET && show_v4)
        {
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip));
            if (strncmp(ip, "127.", 4) == 0)
            {
                tag = "[Loopback IPv4]"; color = YELLOW;
            }
            else
            {
                tag = "[Private IPv4] "; color = GREEN;
            }
        }
        else if (fam == AF_INET6 && show_v6)
        {
            struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)ifa->ifa_addr;
            inet_ntop(AF_INET6, &sa6->sin6_addr, ip, sizeof(ip));
            if (strcmp(ip, "::1") == 0)
            {
                tag = "[Loopback IPv6]"; color = YELLOW;
            }
            else if (strncmp(ip, "fd", 2)==0||strncmp(ip,"FC",2)==0)
            {
                tag = "[ULA IPv6]      "; color = GREEN;
            }
            else if (strncmp(ip, "fe80", 4)==0)
            {
                continue; // skip link-local
            }
            else
            {
                tag = "[Global IPv6]   "; color = BLUE;
            }
        }
        else
        {
            continue;
        }

        printf("  %s%-18s %-42s (%s)%s\n",
               color, tag, ip, ifa->ifa_name, RESET);
    }
    freeifaddrs(ifaddr);
}

void print_public_ips(int show_v4, int show_v6)
{
    printf(WHITE "\n== Public IP Addresses ==\n" RESET);
    if (show_v4)
    {
        char *ip4 = NULL;
        if (get_public_ip("https://ipv4.icanhazip.com", &ip4))
        {
            printf(CYAN "  %-18s %-42s" RESET "\n", "[Public IPv4]", ip4);
            free(ip4);
        }
    }
    if (show_v6)
    {
        char *ip6 = NULL;
        if (get_public_ip("https://ipv6.icanhazip.com", &ip6))
        {
            printf(CYAN "  %-18s %-42s" RESET "\n", "[Public IPv6]", ip6);
            free(ip6);
        }
    }
}

void show_help(const char *prog)
{
    printf("Usage: %s [OPTIONS]\n\n", prog);
    printf("Display your computer's IP addresses (local and/or public).\n\n");
    printf("Options:\n");
    printf("  -h, --help                Show this help message and exit\n");
    printf("  -l, --local               Show only local IP addresses\n");
    printf("  -p, --public              Show only public IP addresses\n");
    printf("  -4                        Show only IPv4 addresses\n");
    printf("  -6                        Show only IPv6 addresses\n");
    printf("  -i, --interface <name>    Show IPs for a specific network interface (e.g. eth0, wlan0)\n");
    printf("\nExamples:\n");
    printf("  %s                 Show all local and public IPs (default)\n", prog);
    printf("  %s -l -4           Show only local IPv4 addresses\n", prog);
    printf("  %s -p -6           Show only public IPv6 address\n", prog);
    printf("  %s -i eno1         Show only IPs from interface 'eno1'\n", prog);
    printf("\nBy default, all IP types (local & public, IPv4 & IPv6) are shown.\n");
}

int main(int argc, char *argv[])
{
    int opt;
    int show_local = 1, show_public = 1;
    int show_v4 = 1, show_v6 = 1;
    char *if_filter = NULL;

    static struct option long_opts[] =
    {
        {"help",      no_argument,       0, 'h'},
        {"local",     no_argument,       0, 'l'},
        {"public",    no_argument,       0, 'p'},
        {"interface", required_argument, 0, 'i'},
        {0,0,0,0}
    };

    while ((opt = getopt_long(argc, argv, "hlp46i:", long_opts, NULL)) != -1)
    {
        switch (opt)
        {
            case 'h': show_help(argv[0]); exit(0);
            case 'l': show_public = 0; break;
            case 'p': show_local = 0; break;
            case '4': show_v6 = 0; break;
            case '6': show_v4 = 0; break;
            case 'i': if_filter = optarg; break;
            default: show_help(argv[0]); exit(1);
        }
    }

    if (show_local)
    {
        print_local_ips(show_v4, show_v6, if_filter);
    }
    if (show_public)
    {
        print_public_ips(show_v4, show_v6);
    }

    return 0;
}
