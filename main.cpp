/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbah <mbah@student.42lyon.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:16:55 by mbah              #+#    #+#             */
/*   Updated: 2026/08/24 22:20:17 by mbah             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include "src/core/user/User.hpp"
#include "src/core/request/Request.hpp"

void testParseCommand(const std::string& input, User* user)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "TESTING: \"" << input << "\"" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try
    {
        Request req(input, user);
        req.debug();
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception caught: " << e.what() << std::endl;
    }
}

User* createTestUser(void)
{
    User* user = new User(42, "127.0.0.1:6667");
    user->setNickName("testuser");
    user->setName("tester");
    user->setFullName("Test User");
    user->setRegistered(true);
    user->setPasswordProvided(true);
    return (user);
}

void deleteTestUser(User* user)
{
    delete user;
}

int main(int argc, char **argv)
{
    //Si un argument est passé, on le teste directement
    if (argc > 1)
    {
        std::string input = argv[1];
        for (int i = 2; i < argc; ++i)
        {
            input += " ";
            input += argv[i];
        }
		
        User* user = createTestUser();
        testParseCommand(input, user);
        deleteTestUser(user);
        return (0);
    }
	else
	{
		std::cout << "Usage: ./ircserv \"PRIVMSG #general :Hello world!\"" << std::endl;
	}
    return (0);
}
