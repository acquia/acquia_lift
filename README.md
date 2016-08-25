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