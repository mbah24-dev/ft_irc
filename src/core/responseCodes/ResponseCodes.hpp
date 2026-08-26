/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseCodes.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/24 13:59:20 by mbah              #+#    #+#             */
/*   Updated: 2026/08/26 16:42:20 by mbah             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_CODES_HPP
# define RESPONSE_CODES_HPP

// ======================== CONNEXION (001-005) ========================

/** @brief Message de bienvenue envoyé après une authentification réussie */
#define RPL_WELCOME 001

/** @brief Informations sur le serveur (nom, version) */
#define RPL_YOURHOST 002

/** @brief Date de création du serveur */
#define RPL_CREATED 003

/** @brief Informations sur les modes supportés */
#define RPL_MYINFO 004

/** @brief Redirection vers un autre serveur */
#define RPL_BOUNCE 005

// ======================== REPONSES COMMANDES (321-366) =================

/** @brief Début de la liste des canaux (commande LIST) */
#define RPL_LIST_START 321

/** @brief Information sur un canal (commande LIST) */
#define RPL_LIST_ENTRY 322

/** @brief Fin de la liste des canaux (commande LIST) */
#define RPL_LIST_END 323

/** @brief Modes actifs d'un canal (commande MODE) */
#define RPL_CHANNEL_MODE_IS 324

/** @brief Aucun sujet défini pour ce canal */
#define RPL_NO_TOPIC 331

/** @brief Sujet du canal (commande TOPIC) */
#define RPL_TOPIC 332

/** @brief Confirmation d'invitation (commande INVITE) */
#define RPL_INVITING 341

/** @brief Liste des utilisateurs sur un canal */
#define RPL_NAMES_LIST 353

/** @brief Fin de la liste des utilisateurs */
#define RPL_END_OF_NAMES 366

// ======================== CODES D'ERREUR (400-500) ====================

/** @brief Réponse WHO (commande WHO) */
#define RPL_WHO_REPLY 352

/** @brief Fin de la liste WHO */
#define RPL_END_OF_WHO 315

/** @brief Erreur : Utilisateur ou canal inexistant */
#define ERR_NO_SUCH_NICK 401

/** @brief Erreur : Canal inexistant */
#define ERR_NO_SUCH_CHANNEL 403

/** @brief Erreur : Utilisateur n'est pas sur ce canal */
#define ERR_USER_NOT_IN_CHANNEL 441

/** @brief Erreur : Vous n'êtes pas sur ce canal */
#define ERR_NOT_ON_CHANNEL 442

/** @brief Erreur : Utilisateur déjà sur le canal */
#define ERR_USER_ON_CHANNEL 443

/** @brief Erreur : Client non enregistré */
#define ERR_NOT_REGISTERED 451

/** @brief Erreur : Pas assez de paramètres pour la commande */
#define ERR_NEED_MORE_PARAMS 461

/** @brief Erreur : Canal plein (limite atteinte) */
#define ERR_CHANNEL_IS_FULL 471

/** @brief Erreur : Mode inconnu */
#define ERR_UNKNOWN_MODE 472

/** @brief Erreur : Canal en mode invite-only */
#define ERR_INVITE_ONLY_CHAN 473

/** @brief Erreur : Vous êtes banni de ce canal */
#define ERR_BANNED_FROM_CHAN 474

/** @brief Erreur : Mauvais mot de passe pour le canal */
#define ERR_BAD_CHANNEL_KEY 475

/** @brief Erreur : Nom de canal invalide */
#define ERR_BAD_CHAN_NAME 479

/** @brief Erreur : Vous n'êtes pas opérateur du canal */
#define ERR_CHAN_OP_PRIVS_NEEDED 482

/** @brief Erreur : Les utilisateurs ne correspondent pas */
#define ERR_USERS_DONT_MATCH 502

// ======================== CODES PERSONNALISES (1000+) =================

/** @brief Erreur : Canal déjà rejoint par l'utilisateur */
#define ERR_CHANNEL_ALREADY_JOINED 1010

/** @brief Confirmation : Canal rejoint avec succès */
#define RPL_CHANNEL_JOINED 1011

/** @brief Confirmation : Canal quitté avec succès */
#define RPL_CHANNEL_LEFT 1012

/** @brief Confirmation : Mode changé avec succès */
#define RPL_MODE_CHANGED 1013

/** @brief Erreur : L'utilisateur n'est pas un opérateur */
#define ERR_NOT_AN_OPERATOR 1014

/** @brief Erreur : L'utilisateur est déjà opérateur */
#define ERR_ALREADY_AN_OPERATOR 1015

#endif
