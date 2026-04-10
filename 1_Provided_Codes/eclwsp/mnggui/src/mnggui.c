/*
** ============================================================================
** Name        : mnggui.c
** Author      : Kai Mueller
** Version     :
** Copyright   : K. Mueller, 20-SEP-2022
** Description : Measurement Network Measurement Gateway GUI Template
** ============================================================================
*/

#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <slope/slope.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>
#include <time.h>
#include "tcpmessage.h"



static GtkWidget *view;
static GtkTextBuffer *st_buf;

static SlopeItem *series1, *series2;
static SlopeScale *scale1;
static GtkWidget *status_line;
static GtkTextBuffer *buffer;
static GtkTextIter iter;

#define CMD_LEN 64
static char Cmd_String[CMD_LEN];
#define STR_MAX 32
static char Server_Name[STR_MAX];

#define N_DATA 300
static double X_Data[N_DATA];
static double Y1_Data[N_DATA];
static double Y2_Data[N_DATA];

static int clientSocket;
static volatile int Server_Connected;
static volatile int Do_Data_Transfer;
static int Per_Thr_Count;

static Srv_Send_struct Meas_Data;
static Clnt_Send_struct Clnt_Data;
static int Clnt_Msg_Size;
static int Srv_Msg_Size;



static void Plot_Init()
{
    int k;

    for (k = 0; k < N_DATA; k++) {
        X_Data[k] = 8.0 * M_PI / N_DATA * k;
        Y1_Data[k] = sin(X_Data[k]);
        Y2_Data[k] = cos(X_Data[k]);
    }
}

static void Plot_Update(unsigned int y0_new, unsigned int y1_new)
{
    int k;

    for (k = 0; k < N_DATA-1; k++) {
        // X_Data[k] += 1.0;
        // Y1_Data[k] *= 1.1;
    	Y1_Data[k] = Y1_Data[k+1];
    	Y1_Data[N_DATA-1] = 3.051757813e-5 * (*(int *)&y0_new);
    	Y2_Data[k] = Y2_Data[k+1];
    	Y2_Data[N_DATA-1] = 3.051757813e-5 * (*(int *)&y1_new);
    }
    slope_scale_remove_item(scale1, series1);
    series1 = slope_xyseries_new_filled("M1", X_Data, Y1_Data, N_DATA, "r-");
    slope_scale_add_item(scale1, series1);
    slope_scale_remove_item(scale1, series2);
    series2 = slope_xyseries_new_filled("M2", X_Data, Y2_Data, N_DATA, "b-");
    slope_scale_add_item(scale1, series2);
    slope_view_redraw(SLOPE_VIEW(view));
}

static int ClientConnect(char *srvname)
{
	struct sockaddr_in serverAddr;
	socklen_t addr_size;
	int rc;

	// Create the socket.
	clientSocket = socket(PF_INET, SOCK_STREAM, 0);
	//Configure settings of the server address
	// Address family is Internet
	serverAddr.sin_family = AF_INET;
	//Set port number, using htons function
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr(srvname);
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
	//Connect the socket to the server using the address
	addr_size = sizeof serverAddr;
	rc = connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);
	Server_Connected = 0;
	if (rc == 0) {
		Server_Connected = 1;
	}
	return rc;
}

static void ClientDisconnect()
{
	if (Server_Connected == 1) {
		Server_Connected = 0;
		close(clientSocket);
	}
}

static void PrintText(char *msg, int option)
{
	GtkTextIter istart, iend;

	switch (option) {
	case 0:
		gtk_text_buffer_insert(buffer, &iter, msg, -1);
		break;
	case 1:
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
				  msg, -1, "bold", NULL);
		break;
	case 2:
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
				  msg, -1, "blue_fg", NULL);
		break;
	case 3:
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
				  msg, -1, "blue_fg", "bold", NULL);
		break;
	default:
		gtk_text_buffer_get_start_iter(buffer, &istart);
		gtk_text_buffer_get_end_iter(buffer, &iend);
		gtk_text_buffer_delete(buffer, &istart, &iend);
		gtk_text_buffer_get_start_iter(buffer, &iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
				  msg, -1, "blue_fg", "bold", NULL);
	}
}


static gboolean PeriodicThread(int *is_connected)
{
	unsigned int n_read, k, y0_new, y1_new;
	char sbuf[128];
	static int anim_count = 0;

	// printf("Periodic Thread\n");
	if (Do_Data_Transfer == 0) {
		// printf("-- no data transfer --\n");
		PrintText("--- Status\n\n", -1);
		switch (anim_count) {
		case 0:
			PrintText(" no data  -\n", 0);
			anim_count = 1;
			break;
		case 1:
			PrintText(" no data  /\n", 0);
			anim_count = 2;
			break;
		case 2:
			PrintText(" no data  |\n", 0);
			anim_count = 3;
			break;
		case 3:
			PrintText(" no data  \\\n", 0);
			anim_count = 0;
			break;
		default:
			anim_count = 0;
		}
	} else {
		if (Server_Connected != 0) {
			Clnt_Data.cmd = 1;
			Clnt_Data.param[0] = 10;
			Clnt_Data.param[1] = 11;
			Clnt_Data.param[2] = 12;

			if (send(clientSocket, &Clnt_Data, Clnt_Msg_Size, 0) < 0) {
				printf("Send failed\n");
			}
			printf("Request sent to server...\n");
			//Read the message from the server into the buffer
			if((n_read = recv(clientSocket, &Meas_Data, Srv_Msg_Size, 0)) < 0) {
				printf("Receive failed\n");
			}
			// Print the received message
			printf("Data received: %d/%d bytes.\n", n_read, Srv_Msg_Size);
			printf("     msg_type: %d\n", Meas_Data.msg_type);
			printf("     nsamples: %d\n", Meas_Data.nsamples);
			printf("       scount: %d\n", Meas_Data.scount);

			PrintText("--- Measurements\n\n", -1);
			for (k = 0; k < Meas_Data.nsamples; k++) {
				printf(" %d:  t=%u  d=6%d\n", k, Meas_Data.stime[k], Meas_Data.sval[k]);
				sprintf(sbuf, " %d:  t=%u  d=%6d\n",
						k, Meas_Data.stime[k], Meas_Data.sval[k]);
				PrintText(sbuf, 0);
			}
		}
		y0_new = Meas_Data.sval[0];
		y1_new = Meas_Data.sval[1];
		Plot_Update(y0_new, y1_new);
	}
	Per_Thr_Count++;
	return TRUE;
}


static void cmd_callback(GtkWidget *widget,
                           GtkWidget *entry )
{
  const gchar *entry_text;
  entry_text = gtk_entry_get_text (GTK_ENTRY(entry));
  strncpy(Cmd_String, entry_text, CMD_LEN);
  Cmd_String[CMD_LEN-1] = '\0';
  gtk_entry_set_text(GTK_ENTRY (entry), "");

  if (!strncmp(Cmd_String, "conn", 4)) {
    if (strlen(Cmd_String) == 4) {
    	strcpy(Server_Name, "127.0.0.1");
    } else if (strlen(Cmd_String) > 11) {
    	strncpy(Server_Name, &Cmd_String[5], STR_MAX);
    	Server_Name[STR_MAX-1] = '\0';
    } else {
    	gtk_text_buffer_set_text(st_buf, "*** invalid server address", -1);
    	return;
    }
    printf("[%s]\n", Server_Name);
    if (ClientConnect(Server_Name) == 0) {
    	gtk_text_buffer_set_text(st_buf, "+++ connected to server...", -1);
    } else {
    	gtk_text_buffer_set_text(st_buf, "*** cannot connected to server", -1);
    }
  } else if (!strncmp(Cmd_String, "disconn", 7)) {
	  ClientDisconnect();
	  gtk_text_buffer_set_text(st_buf, "+++ connected closed.", -1);
  } else if (!strncmp(Cmd_String, "txstart", 7)) {
	  gtk_text_buffer_set_text(st_buf, "+++ transfer begin...", -1);
	  Do_Data_Transfer = 1;
  } else if (!strncmp(Cmd_String, "txstop", 6)) {
	  gtk_text_buffer_set_text(st_buf, "+++ transfer stopped.", -1);
	  Do_Data_Transfer = 0;
  } else {
	  gtk_text_buffer_set_text(st_buf, "*** invalid command!", -1);
  }
}



int main(int argc, char *argv[]) {
  GtkWidget *window;
  GtkWidget *vbox;

  GtkWidget *menubar;
  GtkWidget *fileMenu;
  GtkWidget *fileMi;
  GtkWidget *quitMi;

  SlopeFigure *figure;

  GtkWidget *textview, *scrolledwindow;
  GtkWidget *textentry;

  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(window), 1000, 800);
  gtk_window_set_title(GTK_WINDOW(window), "MNG Control");
  gtk_container_set_border_width(GTK_CONTAINER(window), 5);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  menubar = gtk_menu_bar_new();
  fileMenu = gtk_menu_new();

  fileMi = gtk_menu_item_new_with_label("File");
  quitMi = gtk_menu_item_new_with_label("Quit");

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMi), fileMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitMi);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMi);
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);
  g_signal_connect(G_OBJECT(window), "destroy",
        G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(quitMi), "activate",
        G_CALLBACK(gtk_main_quit), NULL);

  status_line = gtk_text_view_new();
  gtk_text_view_set_editable (GTK_TEXT_VIEW(status_line), FALSE);
  gtk_text_view_set_monospace (GTK_TEXT_VIEW(status_line), TRUE);
  st_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(status_line));
  gtk_text_buffer_set_text(st_buf, "Status", -1);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (status_line), 10);
  gtk_box_pack_start(GTK_BOX(vbox), status_line, FALSE, TRUE, 0);

  view   = slope_view_new();
  figure = slope_figure_new();
  slope_view_set_figure(SLOPE_VIEW(view), figure);

  gtk_box_pack_start(GTK_BOX(vbox), view, FALSE, FALSE, 0);

  Plot_Init();
  scale1 = slope_xyscale_new_axis("time", "measurements", "realtime");
  slope_figure_add_scale(SLOPE_FIGURE(figure), scale1);

  series1 = slope_xyseries_new_filled("M1", X_Data, Y1_Data, N_DATA, "r-");
  slope_scale_add_item(scale1, series1);
  series2 = slope_xyseries_new_filled("M2", X_Data, Y2_Data, N_DATA, "b-");
  slope_scale_add_item(scale1, series2);

  textview = gtk_text_view_new();
  gtk_text_view_set_editable (GTK_TEXT_VIEW(textview), FALSE);
  gtk_text_view_set_monospace (GTK_TEXT_VIEW(textview), TRUE);
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
  // gtk_text_buffer_set_text(buffer, sample_text, -1);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (textview), 10);
  gtk_text_buffer_create_tag(buffer, "blue_fg", "foreground", "blue", NULL);
  gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
  gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
  gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
		  "Status Messages:\n", -1, "blue_fg", "bold", NULL);

  scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(scrolledwindow), textview);
  gtk_box_pack_start(GTK_BOX(vbox), scrolledwindow, TRUE, TRUE, 0);

  textentry = gtk_entry_new();
  gtk_entry_set_max_length (GTK_ENTRY (textentry), CMD_LEN);
  g_signal_connect (G_OBJECT (textentry), "activate",
		      G_CALLBACK (cmd_callback),
		      (gpointer) textentry);

  gtk_box_pack_start(GTK_BOX(vbox), textentry, FALSE, TRUE, 0);

  Clnt_Msg_Size = sizeof(Clnt_Send_struct);
  Srv_Msg_Size =  sizeof(Srv_Send_struct);

  Server_Connected = 0;
  Do_Data_Transfer = 0;
  g_timeout_add(500, (GSourceFunc) PeriodicThread, (gpointer)&Server_Connected);


  gtk_widget_show_all(window);
  gtk_main();
  ClientDisconnect();
  printf("Client disconnected.\n");
  return 0;
}
