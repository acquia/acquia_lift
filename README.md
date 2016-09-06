# Acquia Lift for Drupal 8

This is an early and development-only branch of the Acquia Lift module for Drupal 8. There is no release yet; please do NOT use this branch unless you are explicitly instructed to.

If you are looking for using Acquia Lift for Drupal 8, please contact your Solution Architect or Customer Success Engineer for help. Thank you!

## How to use the Lift API

```
/** @var \Drupal\acquia_lift\Lift\APILoader $liftAPILoader */
$liftAPILoader = \Drupal::service('acquia_lift.lift.api_loader');
$liftAPILoader->getLiftClient()->ping();
```

Make sure you wrap it in a try { ... } catch (\Exception $e) { } routine as it
could throw errors if it is unable to make a connection. If you want to know
what you can do with the API, see documentation at https://github.com/acquia/lift-sdk-php

You can also use Drupal API's to create a new Slot Config Entity and then it
will appear in the list. Please see the Drupal documentation on how to do this.

## How to run all the tests (Simpletests and PHPUnit tests)

Replace the url with the url corresponding to the folder you are currently in or
however you have defined your webserver. the verbose and browser options.
Required for the web tests that exist in the module.

```
php core/scripts/run-tests.sh --url http://drupal8lift3.dev --class "Drupal\acquia_lift\Tests\SettingsTest"
```

## How to run only PHPUnit tests using the PHPunit binary

Follow the guide here to get started: https://www.drupal.org/node/2116263

```
cd core
../vendor/bin/phpunit --group acquia_lift
```
