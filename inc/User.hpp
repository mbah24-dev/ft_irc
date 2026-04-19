/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   User.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:16:47 by mbah              #+#    #+#             */
/*   Updated: 2026/04/19 16:13:49 by mbah             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef USER_HPP
# define USER_HPP

# include <cstddef>
# include <string>

/**
 * @class User
 * @brief Represente un client connecte au serveur.
 */
class User
{
	private:
		int         _fd;
		std::string _host;
		std::string _inputBuffer;

	public:
		/**
		 * @brief Construit un client vide.
		 */
		User();

		/**
		 * @brief Construit un client associe a une socket et un hote.
		 */
		User(int fd, const std::string &host);

		/**
		 * @brief Detruit le client.
		 */
		~User();

		/**
		 * @brief Retourne le descripteur du client.
		 */
		int getFd() const;

		/**
		 * @brief Retourne l'adresse d'origine du client.
		 */
		const std::string &getHost() const;

		/**
		 * @brief Retourne le tampon d'entree en lecture seule.
		 */
		const std::string &getInputBuffer() const;

		/**
		 * @brief Retourne le tampon d'entree en modification.
		 */
		std::string &getInputBuffer();

		/**
		 * @brief Remplace le descripteur du client.
		 */
		void setFd(int fd);

		/**
		 * @brief Remplace l'adresse du client.
		 */
		void setHost(const std::string &host);

		/**
		 * @brief Ajoute des donnees recues au tampon d'entree.
		 */
		void appendInput(const char *data, std::size_t size);

		/**
		 * @brief Vide completement le tampon d'entree.
		 */
		void clearInputBuffer();
};

#endif

