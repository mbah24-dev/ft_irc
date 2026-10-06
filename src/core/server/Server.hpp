/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zcherif <zcherif@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:16:44 by mbah              #+#    #+#             */
/*   Updated: 2026/10/06 13:49:40 by zcherif          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

# include <arpa/inet.h>
# include <cerrno>
# include <cstddef>
# include <csignal>
# include <cstdlib>
# include <cstring>
# include <fcntl.h>
# include <iostream>
# include <map>
# include <netinet/in.h>
# include <poll.h>
# include <stdexcept>
# include <string>
# include <sys/socket.h>
# include <unistd.h>
# include <vector>
# include <ctime>
#include <climits>

# include "User.hpp"
# include "Channel.hpp"
# include "Request.hpp"
# include "../../utils/config.hpp"

/**
 * @class Server
 * @brief Gere le socle reseau du serveur IRC.
 */
class Server
{
    public:
        /** Map associant un descripteur de fichier à un utilisateur */
        typedef std::map<int, User>     ClientRegistry;
        
        /** Map associant un nom de canal à un canal */
        typedef std::map<std::string, Channel> ChannelRegistry;

    public:
        Server(char** arguments);
        ~Server(void);

    // ======================== GETTERS ========================
    public:
        std::string                     getPassword(void) const;
        int                             getListeningPort(void) const;
        const ChannelRegistry&          getActiveChannels(void) const;

    // ======================== MÉTHODES PUBLIQUES ========================
    public:
        void                            startEventLoop(void);
        void                            sendMessage(const std::string& message, int clientSocket);
        void                            sendChannelError(User* client, int code,
                                  const std::string& channelName,
                                  const std::string& message);
        
        ClientRegistry::iterator        findUserByNickname(const std::string& nickname);
        Channel*                        findChannel(const std::string& channelName);

        void                            disconnectClient(User* user, const std::string& message);
        void							removeClientFromChannels(User* user, const std::string& message);
        void							removeClientFromPolling(int clientSocket);
		void							broadcast(const std::string& message, User* sender, Channel& channel);
		void							sendFormattedResponse(const Request& clientRequest, int responseCode);


    // ======================== MÉTHODES PRIVÉES ========================
    private:
        void                            initializeServerSocket(void);
        
        void                            acceptNewClient(void);
        void                            processClientRequest(int pollIndex);
        void                            handleCommand(const std::string& rawData, int clientSocket);
        void                            flushClientOutput(int clientSocket);
        
        void                            addToPolling(int clientSocket);
        void                            removeFromPolling(int index);

        void                            executeCommand(const Request& req);
        bool							 executeAuthCommand(const Request& req);
        bool							 executeMessageCommand(const Request& req);
        bool							 executeChannelCommand(const Request& req);
        bool							 executeAdminCommand(const Request& req);
		std::string						buildResponseMessage(const Request& clientRequest, int responseCode);
        std::string						buildModeResponse(const Request& req, int code) const;
        std::string						buildChannelResponse(const Request& req, int code) const;
        std::string						buildUserResponse(const Request& req, int code) const;
		std::string						intToString(int number) const;

    // ======================== COMMANDES IRC ========================
    private:
        // --- Authentification ---
        void                            handleCapCommand(const Request& req);
        void                            handlePassCommand(const Request& req);
		
        void                            handleNickCommand(const Request& request);
        bool                            validateNickCommand(const Request& request);
		bool							containsForbiddenChars(const std::string& nickname) const;
		void							checkRegistrationComplete(User* user);
		
        void                            handleUserCommand(const Request& req);

        // --- Communication ---
        void                            handlePingCommand(const Request& req);
        void                            handlePongCommand(const Request& req);
        void                            handlePrivmsgCommand(const Request& req);
        void                            handleNoticeCommand(const Request& req);
        void                            deliverMessage(User* sender, const std::string& target,
                                const std::string& text,
                                const std::string& command,
                                bool reportErrors);

        // --- Canaux ---
        void                            handleJoinCommand(const Request& req);
        void                            joinChannel(User* client, const std::string& channelName,
                                const std::string& key);
        bool                            validateChannelJoin(User* client, Channel* channel,
                                    const std::string& key);
        void                            sendChannelNames(User* client, const Channel* channel);
        void                            handlePartCommand(const Request& req);
        void                            removeUserFromChannel(Channel* channel, User* user);
        void                            handleTopicCommand(const Request& req);
        void                            sendChannelTopic(User* client, const Channel* channel);
        void                            updateChannelTopic(User* client, Channel* channel,
                                   const std::string& topic);
        void                            handleListCommand(const Request& req);
        void                            sendChannelListEntry(User* client, const Channel& channel);
        void                            handleNamesCommand(const Request& req);
        void                            handleWhoCommand(const Request& req);
        void                            sendWhoForAll(User* requester, bool operatorsOnly);
        void                            sendWhoReply(User* requester, const std::string& channelName,
                                 User* subject, const Channel* channel);
        void                            handleInviteCommand(const Request& req);
        void                            processInvite(User* inviter, const std::string& nickname,
                                  const std::string& channelName);
        void                            handleKickCommand(const Request& req);
        void                            processKick(User* kicker, const std::string& channelName,
                                const std::string& nickname,
                                const std::string& reason);
        void                            handleModeCommand(const Request& req);
        bool                            prepareModeCommand(const Request& req, Channel*& channel);
        bool                            validateModeArguments(User* client, Channel* channel,
                                      const std::string& channelName,
                                      const std::string& modes,
                                      const std::vector<std::string>& params);
        bool                            validateModeParameter(User* client, Channel* channel,
                                      const std::string& channelName,
                                      char mode, const std::string& value);
        bool                            applyChannelMode(Channel* channel, char mode,
                                 char sign, const std::string& value);
        bool                            applyOperatorMode(Channel* channel, char sign,
                                  const std::string& nickname);
        bool                            applyKeyMode(Channel* channel, char sign,
                                 const std::string& key);
        bool                            applyLimitMode(Channel* channel, char sign,
                                   const std::string& value);
        void                            applyModeChanges(Channel* channel,
                                 const std::string& modes,
                                 const std::vector<std::string>& params,
                                 std::string& changedModes,
                                 std::string& changedParams);

        // --- Opérateur ---
        void                            handleOperCommand(const Request& req);
        void                            handleKillCommand(const Request& req);

        // --- Spéciales ---
        void                            handleGlobopsCommand(const Request& req);
        void                            handleShowtimeCommand(const Request& req);

        // --- Déconnexion ---
        void                            handleQuitCommand(const Request& req);

    // ======================== EXCEPTIONS ========================
    public:
        class IncorrectPortValue : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Port must be between 1 and 65535.");
                }
        };

        class InvalidPassword : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Password cannot be empty.");
                }
        };

        class CreateSocketError : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Failed to create socket.");
                }
        };

        class SetSocketOptionError : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Failed to set socket option.");
                }
        };

        class BindSocketError : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Failed to bind socket to port.");
                }
        };

        class ListenSocketError : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Failed to listen on socket.");
                }
        };

        class AcceptSocketError : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Failed to accept connection.");
                }
        };

        class PollFailedError : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Poll failed.");
                }
        };

        class ReceiveMessageFailed : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Failed to receive message.");
                }
        };

        class FdPollFullError : public std::exception
        {
            public:
                virtual const char* what() const throw()
                {
                    return ("Error: Poll array is full.");
                }
        };

    private:
        // --- Authentification ---
        std::string         _connectionPassword;    /**< Mot de passe requis pour se connecter */
        
        // --- Réseau ---
        int                 _listeningPort;         /**< Port sur lequel le serveur écoute */
        int                 _serverSocket;          /**< Socket du serveur pour les connexions entrantes */
        
        // --- Gestion des événements ---
        struct pollfd       _eventPolling[SOMAXCONN];  /**< Tableau des descripteurs surveillés par poll() */
        nfds_t              _activeDescriptors;     /**< Nombre de descripteurs actifs dans le tableau */
        
        // --- Données ---
        ClientRegistry      _connectedClients;      /**< Map des clients connectés (FD -> User) */
        ChannelRegistry     _activeChannels;        /**< Map des canaux actifs (nom -> Channel) */
        
        // --- Métadonnées ---
        std::string         _startupTimestamp;      /**< Date et heure de démarrage du serveur */
};

#endif
