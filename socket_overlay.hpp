#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>
#include <unistd.h>

/*
 * Written in modern C++, this lightweight library provides an object overlay for using POSIX sockets.
 * The aim is to create a simple layer over the built-in socket component so as to facilitate its use, and provide common utility functions.
 *
 * @author Idris Ayouaz, Antoine Sébert
 * @since 2.0
 * @todo split code in server and client with abstract_node for ingeritance
 */

namespace socket_overlay {
	enum node_type {
		CLIENT,
		SERVER
	};
	class socket_overlay {
		/* ATTRIBUTES */
			private:
				static unsigned int instances_count;
				// socket principal
				int socket_ = 0,
					sin_size = 0,
					yes = 1,
					// valeur de connexion maximum en simultanées 10 par défaut
					max_simultaneous = 0;
				// sockets secondaires qui sont cr?s lors d'une connexion avec un client
				std::vector<int> socketfd_ = {};
				// structure contenant les information de connexion distante
				struct sockaddr_in their_addr = nullptr,
				// structure contenant les information de connexion local
					sockaddr_in my_addr = nullptr;
				std::mutex mtx = nullptr;
		/* METHODS */
			public:
				// constructor
					socket_overlay(unsigned int _max_simultaneous) : max_simultaneous(_max_simultaneous), socket(socket(AF_INET, SOCK_STREAM, 0)) {
						if(socket_ == -1)
							print_error_and_exit("SOCKET CREATION");
						if(setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) == -1)
							print_error_and_exit("SOCKET INITIALIZATION");
						++instances_count;
					}
				// destructor
					~socket_overlay() noexcept { --instances_count; }
				// getters
					int getSocket(unsigned int id){ return (id < 0 ? socket_ : socketfd_[id]); }
					const std::vector& getSockets(){ return &socketfd_; }
					size_t size(){ return socketfd_.size(); }
				// setters
					// Initialisation de la structure sockadr_in. Preciser le port local (utile uniquement pour les serveurs)
					void setMyAddr(int port_){
						//pour un serveur
						my_addr.sin_addr.s_addr = INADDR_ANY;							// adresse, devrait ?re converti en reseau mais est egal ?0
						my_addr.sin_family = AF_INET;									// type de la socket
						my_addr.sin_port = htons(port_);								// port, converti en reseau
						bzero(&(my_addr.sin_zero), 8);									// mise a zero du reste de la structure
					}
					// Initialisation de la structure sockadr_in. Preciser le port et l'adresse ip distante
					void setTheirAddr(std::string addr, int port_){
						their_addr.sin_addr.s_addr = inet_addr((char*)addr.c_str());	// ip
						their_addr.sin_family = AF_INET;								// type de la socket
						their_addr.sin_port = ntohs(port_);								// port, converti en reseau
						bzero(&(their_addr.sin_zero), 8);								// mise a zero du reste de la structure
					}
					void envoyer(char * c, int sockfd){
						if(sockfd != -1) {
							if(send(socketfd_[sockfd], c, strlen(c), 0) == -1)
								print_error_and_exit("SEND");
						}
						else if(send(socket_, c, strlen(c), 0) == -1)
								print_error_and_exit("SEND");
					}
				// network
					// Creation d'une connexion
					void connection(){
						if(connect(socket_, (struct sockaddr*)&their_addr, sizeof(struct sockaddr)) == -1)
							print_error_and_exit("CONNEXION");
					}
					// -1 pour la socket main
					std::string recevoir(int tampon_, int sockfd){
						char* retour = new char[tampon_];
						int tmp = (sockfd == -1 ? recv(socket_, retour, tampon_, 0) : recv(socketfd_[sockfd], retour, tampon_, 0));
						if(tmp == -1)
							print_error_and_exit("RECEIVE");
						if(tmp >= tampon_) {
							cout << "ERREUR BILLY!!" << endl;
							return NULL;
						}
						retour[tmp] = '\0';

						std::string string_retour = retour;
						delete retour;

						return string_retour;
					}
					// Utiliser la fonction bind avec une socket ouverte sur l'exterieur (initialisation de socketfd_)
					void binder() {
						//pour un serveur
						if(bind(socket_, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
							print_error_and_exit("BIND");
					}
					// Utiliser la fonction listen qui check si un client tente de se connecter
					void ecouter(){
						if(listen(socket_, max_simultaneous) == -1)
							print_error_and_exit("LISTEN");
					}
					// Accepter la connexion
					int accepter(){
						sin_size = sizeof(struct sockaddr_in);
						mtx.lock();
						for(unsigned int i = 0; i < socketfd_.size(); ++i) {
							if(socketfd_[i] == -1) {
								socketfd_[i] = accept(socket_, (struct sockaddr *)&their_addr, (socklen_t*)&sin_size);
								int sortie = i;
								if(socketfd_[i] == -1)
									print_error_and_exit("ACCEPT");
								mtx.unlock();
								printf("server: (recyclage) got connection from %s\n", inet_ntoa(their_addr.sin_addr));
								return sortie;
							}
						}
						socketfd_.push_back(accept(socket_, (struct sockaddr *)&their_addr, (socklen_t*)&sin_size));
						int sortie = socketfd_.size() - 1;
						if(socketfd_[socketfd_.size() - 1] == -1)
							print_error_and_exit("ACCEPT");
						mtx.unlock();
						printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));
						return sortie;
					}
					void fermer(int sockfd){
						if(sockfd != -1) {
							if(close(socketfd_[sockfd]) == -1)
								print_error_and_exit("CLOSE");
							socketfd_[sockfd] = -1;
							printf("server: got deconnexion\n");
							return;
						}
						else {
							if(close(socket_) == -1) {
								perror("CLOSE : ");
								exit(1);
							}

							printf("server: got deconnexion\n");
						}
					}
					// envoie en broadcast un message ?un vecteur de sockets
					void envoyerGroup(char * c, std::vector<int> destinataires){
						for(const int& element : destinataires) {
							if(send(element, c, strlen(c), 0) == -1)
								print_error_and_exit("SEND");
						}
					}
				protected:
					void print_error_and_exit(std::string operation_name) {
						perror("[ERROR] " + operation_name + " : ");
						exit(1);
					}
	};
}

#endif