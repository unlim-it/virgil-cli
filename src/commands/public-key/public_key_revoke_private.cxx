/**
 * Copyright (C) 2015 Virgil Security Inc.
 *
 * Lead Maintainer: Virgil Security Inc. <support@virgilsecurity.com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     (1) Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *     (2) Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *
 *     (3) Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <tclap/CmdLine.h>

#include <virgil/sdk/ServicesHub.h>

#include <cli/version.h>
#include <cli/pair.h>
#include <cli/util.h>
#include <cli/DescUtils/all.h>

namespace vsdk = virgil::sdk;
namespace vcrypto = virgil::crypto;
namespace vcli = virgil::cli;

#ifdef SPLIT_CLI
#define MAIN main
#else
#define MAIN public_key_revoke_private_main
#endif

int MAIN(int argc, char** argv) {
    try {
        std::vector<std::string> examples;
        examples.push_back("Revoke a chain of private Virgil Cards with confirmed identities connected by "
                           "public-key-id from Public Keys Service:\n"
                           "virgil public-key-revoke-private -a <card_id> -k alice/private.key -f "
                           "alice/private-main-validated-identity.txt -f "
                           "alice/private-reserve-validated-identity.txt\n\n");

        examples.push_back("Revoke a chain of private Virgil Cards with unconfirmed identities connected by "
                           "public-key-id from Public Keys Service:\n"
                           "virgil public-key-revoke-private -a <card_id> -k alice/private.key -d "
                           "email:alice_main@domain.com -d email:alice_reserve@domain.com\n\n");

        examples.push_back("Revoke a chain of private Virgil Cards with unconfirmed identities and obfuscator identity"
                           " value and/or type connected by public-key-id from Public Keys Service:\n"
                           "virgil public-key-revoke-private -a <card_id> -k alice/private.key -d "
                           "<obfuscator_type>:<obfuscator_value_1> -d <obfuscator_type>:<obfuscator_value_2>\n\n");

        std::string descriptionMessage =
            virgil::cli::getDescriptionMessage(vcli::kPublicKeyRevokePrivate_Description, examples);

        // Parse arguments.
        TCLAP::CmdLine cmd(descriptionMessage, ' ', virgil::cli_version());

        TCLAP::ValueArg<std::string> publicKeyIdArg(vcli::kPublicKeyId_ShortName, vcli::kPublicKeyId_LongName,
                                                    vcli::kPublicKeyId_Description, true, "",
                                                    vcli::kPublicKeyId_TypeDesc);

        TCLAP::ValueArg<std::string> cardIdArg(vcli::kCardId_ShortName, vcli::kCardId_LongName,
                                               vcli::kCardId_Description, true, "", vcli::kCardId_TypeDesc);

        TCLAP::MultiArg<std::string> identityArg("d", "identity", "User identifier for Private Virgil Card with "
                                                                  "unconfirmed identity",
                                                 true, "arg");

        TCLAP::MultiArg<std::string> validatedIdentityArg(
            vcli::kValidatedIdentity_ShortName, vcli::kValidatedIdentity_LongName,
            vcli::kPrivateValidatedIdentity_Description, true, vcli::kValidatedIdentity_TypeDesc);

        TCLAP::ValueArg<std::string> privateKeyArg(vcli::kPrivateKey_ShortName, vcli::kPrivateKey_LongName,
                                                   vcli::kPrivateKey_Description, true, "", vcli::kPrivateKey_TypeDesc);

        TCLAP::ValueArg<std::string> privateKeyPasswordArg(
            vcli::kPrivateKeyPassword_ShortName, vcli::kPrivateKeyPassword_LongName,
            vcli::kPrivateKeyPassword_Description, false, "", vcli::kPrivateKeyPassword_TypeDesc);

        TCLAP::SwitchArg verboseArg(vcli::kVerbose_ShortName, vcli::kVerbose_LongName, vcli::kVerbose_Description,
                                    false);

        cmd.add(verboseArg);
        cmd.add(privateKeyPasswordArg);
        cmd.add(privateKeyArg);
        cmd.xorAdd(validatedIdentityArg, identityArg);
        cmd.add(cardIdArg);
        cmd.add(publicKeyIdArg);
        cmd.parse(argc, argv);

        std::string pathPrivateKey = privateKeyArg.getValue();
        vcrypto::VirgilByteArray privateKey = vcli::readPrivateKey(pathPrivateKey);
        vcrypto::VirgilByteArray privateKeyPassword;
        if (privateKeyPasswordArg.isSet()) {
            privateKeyPassword = vcrypto::str2bytes(privateKeyPasswordArg.getValue());
        } else {
            privateKeyPassword = vcli::setPrivateKeyPass(privateKey);
        }
        vsdk::Credentials credentials(privateKey, privateKeyPassword);

        vcli::ConfigFile configFile = vcli::readConfigFile();
        vsdk::ServicesHub servicesHub(configFile.virgilAccessToken, configFile.serviceUri);

        if (validatedIdentityArg.isSet()) {
            std::vector<vsdk::dto::ValidatedIdentity> validatedIdentities;
            std::vector<std::string> validatedIdentityFiles = validatedIdentityArg.getValue();
            for (const auto& validatedIdentityFile : validatedIdentityFiles) {
                vsdk::dto::ValidatedIdentity validatedIdentity = vcli::readValidateIdentity(validatedIdentityFile);
                validatedIdentities.push_back(validatedIdentity);
            }

            servicesHub.publicKey().revoke(publicKeyIdArg.getValue(), validatedIdentities, cardIdArg.getValue(),
                                           credentials);
        } else {
            // identityArg.isSet
            // revoke for Private Virgil Card with unconfirmed identity
            std::vector<std::string> identitiesStr = identityArg.getValue();
            std::vector<vsdk::dto::Identity> identities;
            for (const auto& identityStr : identitiesStr) {
                auto identityPair = vcli::parsePair(identityStr);
                std::string recipientType = identityPair.first;
                std::string recipientValue = identityPair.second;
                vsdk::dto::Identity identity(recipientValue, recipientType);
                identities.push_back(identity);
            }

            servicesHub.publicKey().revokeNotValid(publicKeyIdArg.getValue(), identities, cardIdArg.getValue(),
                                                   credentials);
        }

        if (verboseArg.isSet()) {
            std::string messageSuccess =
                "Card(s) with public-key-id:" + publicKeyIdArg.getValue() + " has been revoked";
            std::cout << messageSuccess << std::endl;
        }

    } catch (TCLAP::ArgException& exception) {
        std::cerr << "public-key-revoke-private. Error: " << exception.error() << " for arg " << exception.argId()
                  << std::endl;
        return EXIT_FAILURE;
    } catch (std::exception& exception) {
        std::cerr << "public-key-revoke-private. Error: " << exception.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}