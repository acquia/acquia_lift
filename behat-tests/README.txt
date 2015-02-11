The Acquia Lift module includes Behat tests to validate the unified toolbar administration experience.
These tests utilize the Behat Drupal Extension: http://behat-drupal-extension.readthedocs.org/

The system requirements are documented here:
  http://behat-drupal-extension.readthedocs.org/en/3.0/requirements.html

To set up your environment to run tests locally:

1.  Download the Selenium stand-alone server JAR file:
    http://selenium-release.storage.googleapis.com/2.44/selenium-server-standalone-2.44.0.jar

2.  Install Composer:
    https://getcomposer.org/doc/00-intro.md#system-requirements

3.  Install project dependencies into a "vendor" folder
   - Navigate to the behat-tests folder
   - Type: php composer.phar install

4.  Copy behat.template.yml and rename it to behat.yml

5.  Update behat.yml settings to match your environment.  You will need
to at least adjust base_url and drupal_root.  Additionally you can copy over
any setting from behat.common.yml and change it to match your configuration
such as a different CSS selector for a particular region.

6.  Create the following Drupal roles:
    Marketer:  This role should have the ability to manage personalized content
    as well as to see the administration menus.  Specifically:
    - Manage personalized content
    - Use the administration pages and help
    - Use the administration toolbar
    - Administer visitor actions
    Nonmarketer:  This role should have the ability to see administration menus
    but not to manage personalized content.  Specifically:
    - Use the administration pages and help
    - Use the administration toolbar
    @todo: Add this role creation into the before/after hooks.

To run tests:

1.  Start the Selenium server
    Type: java -jar /Path/to/Downloaded/selenium-server-standalone-2.44.0.jar

2.  Navigate to the behat-tests directory and type:
    bin/behat

Special notes and considerations:
By default, tests will run using the Firefox browser.  There is a Selenium
bug with Firefox version 35 that will prevent tests from properly executing.
If you plan to utilize Firefox, ensure that you have are not testing with
version 35.  See: https://code.google.com/p/selenium/issues/detail?id=8390
