# WiPad

## Overview
Wireless Padlock or WiPad is an attempt at creating a smart padlock that grants access to the premises of a critical facility to authorized users exclusively.

A user has to go through a seamless registration procedure once for them to be recognized in future access attempts. Once a user is registered, their smart lock interaction history will continue to be tracked for the lifetime of the padlock or until a user with administrative rights manually removes them from the registered users' database.

WiPad runs on an nRF52832 SoC and can only be communicated with wirelessly via BLE.

## Toolchain
WiPad was created using IAR Embedded Workbench for ARM 9.10.2. Future porting efforts are in the pipeline to
transition to more easily accessible toolchains.

## Testing apparatus
WiPad was deployed and tested using an Android 8.1.0 device running an nRF connect mobile app.

## Description
Upon registration, a user is granted a key to use in future access attempts. WiPad offers 4 types of keys for regular users and a special key for its Admin:
* **One-time key**: Expires as soon as it's been used for the first time.
* **Count-restricted key**: Expires after a predefined number of times it's been used.
* **Unlimited key**: Never expires.
* **Time-restricted key**: Expires after a predefined time span has elapsed since it's been used for the first time.
* **Admin key**: Never expires and can only be used by WiPad's Admin.

WiPad's Admin is in many ways a super user that dictates the rules every other user will have to abide by.

**Admin's role**: WiPad's Admin is responsible for adding users to the system's database for them to be identified upon actually trying to use it. Failing to do so will result in users getting rejected for not being recognized by the device. This recognition relies on a universally unique 8-digit number that every user should have and is expected to know when interacting with WiPad. A user's key type is also assigned to them by the Admin when they're first added to the device's database.

**Admin commands**: WiPad's Admin can either add a new user or request information about a registered user's key type and status. These are respectively achieved by typing in the following commands:
* Add new user with a one-time key: mkusi********k1
* Add new user with a count-restricted key: mkusi********k2cXXXX, where XXXX is to be replaced by the number of times this user should be allowed to use this key before it expires.
* Add new user with an unlimited key: mkusi********k3
* Add new user with a time-restricted key: mkusi********k4tXXXX, where XXXX is to be replaced by the maximum amount of time in minutes that this key should be allowed to remain active once it's been used for the first time.
* Add new user with an Admin key: mkusi********k5
* Request user's key type and status: mkud -i ********

*Notes*:
* The 8 consecutive asterisks refer to a user's universally unique 8-digit Id number.
* Adding a new user with an Admin key will automatically award that user Admin status.
* WiPad users are expected to enable notifications on the Status characteristics of all GATT services they interact with. Failing to do so will result in triggering a flashing LED pattern and halting the whole interaction until notifications are enabled.

**Registration**: When a user interacts with WiPad for the first time, they are expected to provide their 8-digit Id first. If WiPad recognizes them, they will be prompted to register a password. Passwords have a specific required format:
* Must be between 8 and 12 characters in length
* Must contain at least one digit
* Must contain at least one special character

**Authentication**: A registered user is always expected to provide their 8-digit Id first followed by their registered password to be given access to their keys. Once a key expires, its holder will be completely removed from the system's database and can therefore no longer be recognized.

**Current time service**: WiPad relies on the Current Time Service to acquire time readings from users' smartphones. A WiPad user is therefore required to have a GATT server with CTS configured on their smartphone.

**Time zone**: WiPad's time management varies slightly depending on the time zone where it's being deployed. This can be set in system_config.h.



