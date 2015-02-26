The Acquia Lift module includes Behat tests to validate the unified toolbar administration experience.
These tests utilize the Behat Drupal Extension: http://behat-drupal-extension.readthedocs.org/

The system requirements are documented here:
  http://behat-drupal-extension.readthedocs.org/en/3.0/requirements.html

To set up your environment to run tests locally:

1.  Download the Selenium stand-alone server JAR file:
    http://selenium-release.storage.googleapis.com/2.44/selenium-server-standalone-2.44.0.jar

2.  Selenium version 2.44 is only compatible with Firefox 33, but not 34 or above. You can download
    Firefox 33 at the following link to a local location of your choice:

    https://ftp.mozilla.org/pub/mozilla.org/firefox/releases/33.1.1/mac/en-US/

3.  Install Composer:
    https://getcomposer.org/doc/00-intro.md#system-requirements

4.  Install project dependencies into a "vendor" folder
    - Navigate to the behat-tests folder
    - Type: composer install

5.  Copy behat.template.yml and rename it to behat.yml

6.  Update behat.yml settings to match your environment.  You will need to at least adjust base_url and
    drupal_root. Additionally you can copy over any setting from behat.common.yml and change it to match
    your configuration such as a different CSS selector for a particular region.

7.  Create the following Drupal roles:
    Marketer:  This role should have the ability to manage personalized content
    as well as to see the administration menus.  Specifically:
    - Manage personalized content
    - Use the administration pages and help
    - Use the administration toolbar
    - Administer visitor actions
    Nonmarketer:  This role should have the ability to see administration menus.
    Specifically:
    - Use the administration pages and help
    - Use the administration toolbar
    But not:
    - Manage personalized content

    @todo: Add this role creation into the before/after hooks.

To run tests:

1.  Start the Selenium server
    Type: java -jar /opt/selenium/selenium-server-standalone-2.44.0.jar -Dwebdriver.firefox.bin="[PATH TO YOUR Firefox.app]/Contents/MacOS/firefox-bin" &

2.  Navigate to the behat-tests directory and type:
    bin/behat
