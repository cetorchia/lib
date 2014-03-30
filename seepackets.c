/* seepackets.c (v2.0)
 *
 * Reads ethernet frames for a period of time. Then,
 * Packet quantities are reported, including download/upload rate.
 * Reports payloads of UDP/TCP packets, optionally.
 *
 * Process is forked in order to analyze packets concurrently.
 *
 * Therefore, you might use this to detect worms, although editing
 * this program to detect patterns might be necessary.
 *
 * Copyright (c) 2009 Carlos E. Torchia
 *
 * This software is licensed under the GNU GPL v2.
 * It can be distributed freely under certain conditions; see fsf.org.
 * There is no warranty, use at your own risk.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <features.h>
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

#define	MAXBUF		1518	// Amount of data to read from socket
#define	DISPLAY_LINE	19	// Number of bytes to display on a line
#define	ANALYZER_PAUSE	1000	// 1 ms = 1000 microsec.

#define	TCP_PROTOCOL	6
#define	UDP_PROTOCOL	17
#define	ICMP_PROTOCOL	1
#define	IGMP_PROTOCOL	2
#define	OSPF_PROTOCOL	89

/* ========== Global variables (minus command line opts.) ====== */

int	data_n=0;
int	sd;			/* Socket for raw frames */
int	fd[2];			/* For interprocess pipe */
int	start_time;		/* Timing purposes. */
int	cpid;			/* child PID */

/* --- traffic count --- */

int	total_n=0;
int	tcp_n=0;
int	udp_n=0;
int	icmp_n=0;
int	igmp_n=0;
int	ospf_n=0;
int	arp_n=0;
int	other_n=0;

/* ========== Command line options ============ */

int	show_payloads=0;
int	show_tcp=0;
int	show_udp=0;
int	show_icmp=0;
int	show_igmp=0;
int	show_ospf=0;
int	show_other=0;
int	show_arp=0;

/* ======= print_usage =========
 * Self-explanatory.
 */

void	print_usage(void)
{
  fprintf(stderr,
          "Usage: seepackets [options] ifname\n"
          "  -p          Show payloads for TCP/UDP packets\n"
          "  -t          Show TCP packets\n"
          "  -u          Show UDP packets\n"
          "  -c          Show ICMP packets\n"
          "  -g          Show IGMP packets\n"
          "  -s          Show OSPF packets\n"
          "  -a          Show ARP packets\n"
          "  -o          Show other packets\n"
          "  -m seconds  Specify maximum duration\n"
          );
}

/* ======= print_time ======= */

void	print_time()
{
  printf("%02d:%02d:%02d ",
         (time(NULL)%86400)/3600,
         (time(NULL)%3600)/60,
         (time(NULL)%3600)%60);
}

/* ====== ip_addr_str =========
 * addr must be in network byte order
 */

char	*ip_addr_str(int addr)
{
  static char	str[17];
  unsigned char	*u=(unsigned char *)&addr;

  sprintf(str,"%d.%d.%d.%d",u[0],u[1],u[2],u[3]);

  return(str);
}

/* ==== mac_addr_str ========
 */

char	*mac_addr_str(unsigned char *addr)
{
  static char	str[18];
  unsigned char	*u=addr;

  sprintf(str,"%.2hhX:%.2hhX:%.2hhX:%.2hhX:%.2hhX:%.2hhX",
          u[0],u[1],u[2],u[3],u[4],u[5]);

  return(str);
}

/* ====== analyze_ip_payload function ==========
 */

void	analyze_ip_payload(char *ip_payload,int ip_payload_length,
			   int	protocol)
{
  int		hlen;
  struct tcphdr	*tcp_header=(struct tcphdr *)ip_payload;
  struct udphdr *udp_header=(struct udphdr *)ip_payload;
  char		*payload;
  int		payload_length;
  unsigned short	sport,dport;
  int		i,j;			/* Payload index */

  /* ---- determine payload length and location ---- */

  if(protocol==TCP_PROTOCOL) {
    hlen=4*tcp_header->doff;
  }
  else if(protocol==UDP_PROTOCOL) {
    hlen=8;
  }
  payload=((char *)ip_payload)+hlen;
  payload_length=ip_payload_length-hlen;
  data_n+=payload_length;

  /* ---- make note of packet if it is of nonzero length ----- */

  if(protocol==TCP_PROTOCOL) {
    sport=ntohs(tcp_header->source);
    dport=ntohs(tcp_header->dest);
  }
  else if(protocol==UDP_PROTOCOL) {
    sport=ntohs(udp_header->source);
    dport=ntohs(udp_header->dest);
  }

  printf("%d ",sport);
  printf("to %d ",dport);
  printf("(%d bytes)\n",payload_length);

  /* --- Print contents of payload... --- */

  if(show_payloads) {

    for(i=0,j=0;i<payload_length;i=j) {
      printf(" ");

      /* -- display in character form -- */

      for(j=i;(j<payload_length)&&(j<i+DISPLAY_LINE);j++) {
        putchar((payload[j]>=0x20)&&(payload[j]<=0x7E)?payload[j]:'.');
      }
      for(;j<i+DISPLAY_LINE;j++) {
        putchar(' ');
      }
      printf("  ");

      /* -- display in integer form -- */

      for(j=i;(j<payload_length)&&(j<i+DISPLAY_LINE);j++) {
        printf("%.2hhX ",payload[j]);
      }
      putchar('\n');
    }
    putchar('\n');
  }
}

/* ===== print_stats =========
 * Prints traffic statistics up to the current time.
 */

void	print_stats(void)
{
  int	duration=time(NULL)-start_time;

  if(data_n>=1) {
    printf("Total TCP/UDP data transferred: %d bytes, %.1f bytes/sec.\n",
           data_n,(double)data_n/duration);
  }

  if(tcp_n>=1) {
    printf("TCP traffic: %d bytes, %.1f bytes/sec.\n",
           tcp_n,(double)tcp_n/duration);
  }

  if(udp_n>=1) {
    printf("UDP traffic: %d bytes, %.1f bytes/sec.\n",
           udp_n,(double)udp_n/duration);
  }

  if(icmp_n>=1) {
    printf("ICMP traffic: %d bytes, %.1f bytes/sec.\n",
           icmp_n,(double)icmp_n/duration);
  }

  if(igmp_n>=1) {
    printf("IGMP traffic: %d bytes, %.1f bytes/sec.\n",
           igmp_n,(double)igmp_n/duration);
  }

  if(ospf_n>=1) {
    printf("OSPF traffic: %d bytes, %.1f bytes/sec.\n",
           ospf_n,(double)ospf_n/duration);
  }

  if(other_n>=1) {
    printf("Other traffic: %d bytes, %.1f bytes/sec.\n",
           other_n,(double)other_n/duration);
  }

  if(arp_n>=1) {
    printf("ARP traffic: %d bytes, %.1f bytes/sec.\n",
           arp_n,(double)arp_n/duration);
  }

  printf("Total traffic: %d bytes, %.1f bytes/sec.\n",
         total_n,(double)total_n/duration);
}

/* ====== quit ================
 * Runs when SIGINT is received (ctrl-c)
 */

void	quit(int k)
{

  if(cpid) {

    /* -- parent proc -- */

    printf("seepackets2: received SIGINT\n");

    close(sd);
    close(fd[1]);

    wait(NULL);		/* --- wait for any children --- */
    exit(0);
  }

  /* -- child proc -- */

  printf("seepackets2: child received SIGINT\n");

  print_stats();
  _exit(0);
}

/* ======= analyzer ===========
 * This part of the program analyzes headers that the other
 * part of the program gives it.
 */

void	analyzer(int fd)
{
  int		n;
  char		buf[MAXBUF];		/* buffer that holds frame */

  struct ethhdr	*ethernet_header;	/* Ethernet frame header */
  char		*ethernet_payload;	/* Payload */

  struct iphdr	*ip_header;		/* IP header */
  char		*ip_payload;		/* Payload for IP packet */
  int		ip_payload_length;

  /* -------- we need to prepare some data pointers ----------- */

  ethernet_header=(struct ethhdr *)buf;
  ethernet_payload=buf+sizeof(struct ethhdr);
  ip_header=(struct iphdr *)ethernet_payload;

  /* -------- start reading packets from big brother (or mother) ---- */

  for(;;) {

    usleep(ANALYZER_PAUSE);		/* Don't use up too more CPU */

    if(read(fd,&n,sizeof(n))!=sizeof(n)) {
      fprintf(stderr,"seepackets2: Couldn't read packet size from pipe\n");
      perror("read");
    }

    if((n=read(fd,buf,n))<=0) {
      fprintf(stderr,"seepackets2: Couldn't read from pipe\n");
      perror("read");
    }

    total_n+=n;

    /* ------- We now analyze header! -------
     * Note: ethernet header pointer is pointing already
     * to beginning of buffer!
     */

    if(ethernet_header->h_proto==htons(ETH_P_IP)) {

      if(ip_header->version==4) {		/* --- IPv4 --- */

        ip_payload_length=n-(sizeof(struct ethhdr)+4*ip_header->ihl);

	if(ip_payload_length!=ntohs(ip_header->tot_len)-4*ip_header->ihl) {
          fprintf(stderr,"seepackets2: packet lied about its size, %d != %d ... corrupt?\n",
                  ip_payload_length,
                  ntohs(ip_header->tot_len)-4*ip_header->ihl);
        }

	ip_payload=((char *)ip_header)+4*ip_header->ihl;

	/* --- We proceed to analyze the payload of the IP datagram --- 
	 * Maybe we'll find some goodies!
         * iphdr.protocol has only 8 bits, don't need htons()
	 */

	if(ip_header->protocol==TCP_PROTOCOL) {
          tcp_n+=n;
          if(show_tcp) {
            print_time();
            printf("TCP ");
            printf("%s ",ip_addr_str(ip_header->saddr));
            printf("to %s ",ip_addr_str(ip_header->daddr));
            analyze_ip_payload(ip_payload,
			       ip_payload_length,
                               ip_header->protocol);
          }
	}

        else if(ip_header->protocol==UDP_PROTOCOL) {
          udp_n+=n;
          if(show_udp) {
            print_time();
            printf("UDP ");
            printf("%s ",ip_addr_str(ip_header->saddr));
            printf("to %s ",ip_addr_str(ip_header->daddr));
            analyze_ip_payload(ip_payload,
			       ip_payload_length,
                               ip_header->protocol);
          }
        }

	else if(ip_header->protocol==ICMP_PROTOCOL) {
          icmp_n+=n;
          if(show_icmp) {
            print_time();
            printf("ICMP ");
            printf("%s ",ip_addr_str(ip_header->saddr));
            printf("to %s\n",ip_addr_str(ip_header->daddr));
          }
        }

        else if(ip_header->protocol==IGMP_PROTOCOL) {
          igmp_n+=n;
          if(show_igmp) {
            print_time();
            printf("IGMP ");
            printf("%s ",ip_addr_str(ip_header->saddr));
            printf("to %s\n",ip_addr_str(ip_header->daddr));
          }
        }

        else if(ip_header->protocol==OSPF_PROTOCOL) {
          ospf_n+=n;
          if(show_ospf) {
            print_time();
            printf("OSPF ");
            printf("%s ",ip_addr_str(ip_header->saddr));
            printf("to %s\n",ip_addr_str(ip_header->daddr));
          }
        }

        else {
          other_n+=n;
          if(show_other) {
            print_time();
            printf("IPv4 ");
            printf("%s ",ip_addr_str(ip_header->saddr));
            printf("to %s\n",ip_addr_str(ip_header->daddr));
          }
        }
      }

      else {
        other_n+=n;
        if(show_other) {
          print_time();
          printf("IPv%d ",ip_header->version);
          printf("%s ",mac_addr_str(ethernet_header->h_source));
          printf("to %s\n",mac_addr_str(ethernet_header->h_dest));
        }
      }
    }

    else if(ethernet_header->h_proto==htons(ETH_P_ARP)) {
      if(show_arp) {
        print_time();
        printf("ARP ",ip_header->version);
        printf("%s ",mac_addr_str(ethernet_header->h_source));
        printf("to %s\n",mac_addr_str(ethernet_header->h_dest));
      }
      arp_n+=n;
    }

    else {
      other_n+=n;
      if(show_other) {
        print_time();
        printf("0x%.4X ",ntohs(ethernet_header->h_proto));
        printf("%s ",mac_addr_str(ethernet_header->h_source));
        printf("to %s\n",mac_addr_str(ethernet_header->h_dest));
      }
    }
  }
}

/* ====== main function ======*/

int	main(int argc,char **argv)
{
  int		i;			/* Counter variable */
  int		n;			/* Number of Bytes received */
  int		r;			/* Return values (bind, etc.) */

  char		buf[MAXBUF];		/* buffer that holds frame */
  char		*ifname;		/* e.g. eth0, from args */
  struct sockaddr_ll	my_addr;	/* Address */

  int		max_duration;		/* Maximum duration */
  int		end_time;		/* Time at which to end */
  int		time_mult=-1;		/* For logic */

  /* -------- catch Ctrl-C signals ----------- */

  signal(SIGINT,quit);

  /* ------- Get arguments: time duration,interface --------- */

  if(argc<2) {
    print_usage();
    exit(-1);
  }

  for(i=1;i<argc-1;i++) {
    if(strcmp(argv[i],"-p")==0) {
      show_payloads=-1;
    }
    else if(strcmp(argv[i],"-t")==0) {
      show_tcp=-1;
    }
    else if(strcmp(argv[i],"-u")==0) {
      show_udp=-1;
    }
    else if(strcmp(argv[i],"-c")==0) {
      show_icmp=-1;
    }
    else if(strcmp(argv[i],"-g")==0) {
      show_igmp=-1;
    }
    else if(strcmp(argv[i],"-s")==0) {
      show_ospf=-1;
    }
    else if(strcmp(argv[i],"-a")==0) {
      show_arp=-1;
    }
    else if(strcmp(argv[i],"-o")==0) {
      show_other=-1;
    }
    else if(strcmp(argv[i],"-m")==0) {
      i++;   /* --- go to duration argument --- */
      max_duration=atoi(argv[i]);
      time_mult=1;
    }
    else {
      print_usage();
      exit(-1);
    }
  }

  if(i==argc-1) {
    ifname=argv[i];	/* --- ifname is the last parameter --- */
  }
  else {
    print_usage();
    exit(-1);
  }

  /* ------- Open socket ---------- */

  sd=socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
  if(sd<0) {
    fprintf(stderr,"seepackets2: Unable to open socket\n");
    perror("socket");
    exit(-1);
  }

  /* ------- prepare my address -------- */

  bzero(&my_addr,sizeof(my_addr));
  my_addr.sll_family=AF_PACKET;			/* Address family */
  my_addr.sll_protocol=htons(ETH_P_ALL);	/* Protocol */
  my_addr.sll_ifindex=if_nametoindex(ifname);	/* Interface */

  if(my_addr.sll_ifindex==0) {
    fprintf(stderr,"seepackets2: there was an error about which the interface name is concerned\n");
    perror("if_nametoindex");
    exit(-1);
  }

  /* ------- Bind interface to socket ------- */

  r=bind(sd,(struct sockaddr *)&my_addr,sizeof(my_addr));

  if(r<0) {
    fprintf(stderr,"seepackets2: Unable to bind %s to socket\n",ifname);
    perror("bind");
    exit(-1);
  }

  /* ---- Separate into two processes ---- */

  start_time=time(NULL);		/* --- we consider this the start time -- */
  end_time=start_time+max_duration;

  if(pipe(fd)) {
    fprintf(stderr,"seepackets2: Can't pipe\n");
    perror("pipe");
    exit(-1);
  }

  cpid=fork();

  if(cpid==0) {
    close(fd[1]);	/* --- close write fd --- */
    close(sd);		/* --- we do not need sd --- */
    analyzer(fd[0]);    /* --- use read fd --- */

    _exit(0);
  }

  else {
    close(fd[0]);	/* --- close read fd --- */
  }

  /* ----- Start reading frames ----- */

  while(time_mult*time(NULL)<end_time) {
    
    /* ------- Receive frame --------- */

    n=recv(sd,buf,MAXBUF,0);

    if(n<=0) {
      fprintf(stderr,"seepackets2: Error receiving frame\n");
      perror("recv");
      exit(-1);
    }

    write(fd[1],&n,sizeof(n));
    write(fd[1],buf,n);
  }

  kill(cpid,SIGINT);
  wait(NULL);

  close(sd);
  close(fd[1]);

  return(0);
}
