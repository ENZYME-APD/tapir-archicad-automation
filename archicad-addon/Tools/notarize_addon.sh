#!/bin/bash

# process command line parameters
P12FilePath=$1
P12FilePassword=$2
CodeSignIdentity=$3
AppleID=$4
AppleIDPassword=$5
TeamID=$6
AddOnBundlePath=$7
TempZipFileName="BundleForNotarization.zip"

# make sure the script exits on error
set -e

# create a temporary keychain and set it as default
security create-keychain -p TempKeyChainPass notarization.keychain
security default-keychain -s notarization.keychain
security unlock-keychain -p TempKeyChainPass notarization.keychain

# import the p12 file into the keychain
security import $P12FilePath -k notarization.keychain -P $P12FilePassword -T /usr/bin/codesign

# make sure that no password prompt will appear
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k TempKeyChainPass notarization.keychain

# sign the bundle
echo "codesign"
echo $AddOnBundlePath
mdls $AddOnBundlePath
echo "codesign end"
/usr/bin/codesign -s $CodeSignIdentity -f -vvv --deep --timestamp --options runtime "$AddOnBundlePath"

# store credentials for notariozation
echo "credentials"
xcrun notarytool store-credentials "addon.notarization" --apple-id $AppleID --password $AppleIDPassword --team-id $TeamID

# create a zip to upload to notarization service
echo "compress"
ditto -c -k --keepParent $AddOnBundlePath $TempZipFileName

# submit the zip to the notarization service
echo "submit"
xcrun notarytool submit $TempZipFileName --wait --keychain-profile "addon.notarization"

# staple the ticket to the bundle
echo "staple"
xcrun stapler staple $AddOnBundlePath

# delete the temporary zip file
rm $TempZipFileName
