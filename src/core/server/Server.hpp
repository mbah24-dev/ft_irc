/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:16:44 by mbah              #+#    #+#             */
/*   Updated: 2026/08/24 22:13:06 by mbah             ###   ########.fr       */
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

# include "User.hpp"

/**
 * @class Server
 * @brief Gere le socle reseau du serveur IRC.
 */
class Server
{
	
};

#endif

