#!/bin/bash

# process command line parameters
P12FilePath=$1
P12FilePassword=$2
CodeSignIdentity=$3
EntitlementsFilePath=$4
AppleID=$5
AppleIDPassword=$6
TeamID=$7
AddOnBundlePath=$8

# make sure the script exits on error
set -e

# create a temporary keychain and set it as default
security create-keychain -p TempKeyChainPass notarization.keychain
security default-keychain -s notarization.keychain
security unlock-keychain -p TempKeyChainPass notarization.keychain

# import the p12 file into the keychain
security import "$P12FilePath" -k notarization.keychain -P $P12FilePassword -T /usr/bin/codesign

# make sure that no password prompt will appear
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k TempKeyChainPass notarization.keychain

# sign the bundle
/usr/bin/codesign -s "$CodeSignIdentity" -f -vvv --deep --timestamp --entitlements "$EntitlementsFilePath" --options runtime "$AddOnBundlePath"

# store credentials for notarization
xcrun notarytool store-credentials "addon.notarization" --apple-id $AppleID --password $AppleIDPassword --team-id $TeamID

# set file name for the zip to send for notarization
TempZipFileName="BundleForNotarization.zip"

# create a zip to upload to notarization service
ditto -c -k --keepParent "$AddOnBundlePath" "$TempZipFileName"

# submit the zip to the notarization service
xcrun notarytool submit "$TempZipFileName" --wait --keychain-profile "addon.notarization"

# staple the ticket to the bundle
xcrun stapler staple "$AddOnBundlePath"

# delete the temporary zip file
rm "$TempZipFileName"
