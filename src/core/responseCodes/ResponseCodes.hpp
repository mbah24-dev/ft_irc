/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseCodes.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 13:59:20 by mbah              #+#    #+#             */
/*   Updated: 2026/08/24 22:13:13 by mbah             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_CODES_HPP
# define RESPONSE_CODES_HPP

typedef enum e_responseCode
{
    // ======================== CONNEXION (001-005) ========================
    // Ces codes sont envoyés lors de la phase d'authentification
    // Ils confirment que le client est bien connecté au serveur
    
    /** @brief Message de bienvenue envoyé après une authentification réussie */
    RPL_WELCOME = 001,
    
    /** @brief Informations sur le serveur (nom, version) */
    RPL_YOURHOST = 002,
    
    /** @brief Date de création du serveur */
    RPL_CREATED = 003,
    
    /** @brief Informations sur les modes supportés */
    RPL_MYINFO = 004,
    
    /** @brief Redirection vers un autre serveur */
    RPL_BOUNCE = 005,
    
    // ======================== REPONSES COMMANDES (321-366) =================
    // Ces codes sont envoyés en réponse aux commandes du client
    // Ils contiennent les informations demandées
    
    /** @brief Début de la liste des canaux (commande LIST) */
    RPL_LIST_START = 321,
    
    /** @brief Information sur un canal (commande LIST) */
    RPL_LIST_ENTRY = 322,
    
    /** @brief Fin de la liste des canaux (commande LIST) */
    RPL_LIST_END = 323,
    
    /** @brief Modes actifs d'un canal (commande MODE) */
    RPL_CHANNEL_MODE_IS = 324,
    
    /** @brief Aucun sujet défini pour ce canal */
    RPL_NO_TOPIC = 331,
    
    /** @brief Sujet du canal (commande TOPIC) */
    RPL_TOPIC = 332,
    
    /** @brief Confirmation d'invitation (commande INVITE) */
    RPL_INVITING = 341,
    
    /** @brief Liste des utilisateurs sur un canal */
    RPL_NAMES_LIST = 353,
    
    /** @brief Fin de la liste des utilisateurs */
    RPL_END_OF_NAMES = 366,
    
    // ======================== CODES D'ERREUR (400-500) ====================
    // Ces codes sont envoyés lorsque le client effectue une action invalide
    // Le message associé explique l'erreur
    
    /** @brief Erreur : Utilisateur ou canal inexistant */
    ERR_NO_SUCH_NICK = 401,
    
    /** @brief Erreur : Canal inexistant */
    ERR_NO_SUCH_CHANNEL = 403,
    
    /** @brief Erreur : Utilisateur n'est pas sur ce canal */
    ERR_USER_NOT_IN_CHANNEL = 441,
    
    /** @brief Erreur : Vous n'êtes pas sur ce canal */
    ERR_NOT_ON_CHANNEL = 442,
    
    /** @brief Erreur : Utilisateur déjà sur le canal */
    ERR_USER_ON_CHANNEL = 443,
    
    /** @brief Erreur : Client non enregistré */
    ERR_NOT_REGISTERED = 451,
    
    /** @brief Erreur : Pas assez de paramètres pour la commande */
    ERR_NEED_MORE_PARAMS = 461,
    
    /** @brief Erreur : Canal plein (limite atteinte) */
    ERR_CHANNEL_IS_FULL = 471,
    
    /** @brief Erreur : Mode inconnu */
    ERR_UNKNOWN_MODE = 472,
    
    /** @brief Erreur : Canal en mode invite-only */
    ERR_INVITE_ONLY_CHAN = 473,
    
    /** @brief Erreur : Vous êtes banni de ce canal */
    ERR_BANNED_FROM_CHAN = 474,
    
    /** @brief Erreur : Mauvais mot de passe pour le canal */
    ERR_BAD_CHANNEL_KEY = 475,
    
    /** @brief Erreur : Nom de canal invalide */
    ERR_BAD_CHAN_NAME = 479,
    
    /** @brief Erreur : Vous n'êtes pas opérateur du canal */
    ERR_CHAN_OP_PRIVS_NEEDED = 482,
    
    /** @brief Erreur : Les utilisateurs ne correspondent pas */
    ERR_USERS_DONT_MATCH = 502,
    
    // ======================== CODES PERSONNALISES (1000+) =================
    // Ces codes sont spécifiques à notre implémentation
    // Ils ne font pas partie du standard IRC mais facilitent la gestion interne
    
    /** @brief Erreur : Canal déjà rejoint par l'utilisateur */
    ERR_CHANNEL_ALREADY_JOINED = 1010,
    
    /** @brief Confirmation : Canal rejoint avec succès */
    RPL_CHANNEL_JOINED = 1011,
    
    /** @brief Confirmation : Canal quitté avec succès */
    RPL_CHANNEL_LEFT = 1012,
    
    /** @brief Confirmation : Mode changé avec succès */
    RPL_MODE_CHANGED = 1013,
    
    /** @brief Erreur : L'utilisateur n'est pas un opérateur */
    ERR_NOT_AN_OPERATOR = 1014,
    
    /** @brief Erreur : L'utilisateur est déjà opérateur */
    ERR_ALREADY_AN_OPERATOR = 1015
};

#endif
