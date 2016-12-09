<?php

namespace Drupal\acquia_lift\Tests;

use Drupal\simpletest\WebTestBase;

/**
 * Test Acquia Lift Toolbar Link.
 *
 * @group Acquia Lift
 */
class LiftLinkTest extends WebTestBase {

  use SettingsDataTrait;

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = array('node', 'acquia_lift', 'menu_ui', 'user', 'toolbar');

  /**
   * {@inheritdoc}
   */
  protected function setUp() {
    parent::setUp();


    // Create Basic page and Article node types.
    if ($this->profile != 'standard') {
      $this->drupalCreateContentType(array(
        'type' => 'page',
        'name' => 'Basic page',
        'display_submitted' => FALSE,
      ));
      $this->drupalCreateContentType(array('type' => 'article', 'name' => 'Article'));
    }
  }

  public function testLinkInToolbar() {
    $permissions = [
      'access toolbar',
      'access acquia lift links',
    ];

    // User to set up acquia_lift.
    $linkUser = $this->drupalCreateUser($permissions);
    $this->drupalLogin($linkUser);

    // Set valid settings
    $this->setValidSettings();

    // Check if Acquia Lift Link is available on the node that we created
    $node = $this->drupalCreateNode();

    // Assert that the Acquia Lift link ID is present in the HTML.
    // This also verifies if the specific render cache is cleared.
    $this->drupalGet($node->toUrl());
    $this->assertRaw('id="openLiftLink"');

  }

  // @todo Figure out why the cache does not clear after changing the config.
  // Given that we (forcefully) clear the cache in the settings page, we can
  // skip this test for now.
  public function testLinkInToolbarAfterConfigChange() {
    $permissions = [
      'access toolbar',
      'access acquia lift links',
    ];

    // User to set up acquia_lift.
    $linkUser = $this->drupalCreateUser($permissions);
    $this->drupalLogin($linkUser);

    // Check if Acquia Lift Link is available on the node that we created
    $node = $this->drupalCreateNode();

    // Assert that the Acquia Lift link ID is not present in the HTML.
    $this->drupalGet($node->toUrl());
    $this->assertNoRaw('id="openLiftLink"');
    // These contexts should be set.
    $this->assertCacheContext('url.path');
    $this->assertCacheContext('user');
    // These tags should be set.
    $this->assertCacheTag('config:acquia_lift.settings');

    // Set valid settings
    $this->setValidSettings();

    // Assert that the Acquia Lift link ID is present in the HTML.
    // This also verifies if the specific render cache is cleared.
    $this->drupalGet($node->toUrl());
    $this->assertRaw('id="openLiftLink"');

    // These tags should be set.
    $this->assertCacheTag('config:acquia_lift.settings');
    // These contexts should be set.
    $this->assertCacheContext('url.path');
    $this->assertCacheContext('user');
    $this->assertCacheContext('url.query_args:_wrapper_format');
    $this->assertCacheContext('url.query_args:my_identity_parameter');
    $this->assertCacheContext('url.query_args:my_identity_type_parameter');
  }

  public function testLinkNotInToolbar() {
    $permissions = [
      'access toolbar'
    ];

    // User to set up acquia_lift.
    $linkUser = $this->drupalCreateUser($permissions);
    $this->drupalLogin($linkUser);

    // Check if Acquia Lift Link is available on the node that we created
    $node = $this->drupalCreateNode();
    $this->drupalGet($node->toUrl());
    // Assert that the Acquia Lift link ID is not present in the HTML.
    $this->assertNoRaw('id="openLiftLink"');

    // Set valid settings
    $this->setValidSettings();
    $this->drupalGet($node->toUrl());
    // Assert that the Acquia Lift link ID is not present in the HTML.
    $this->assertNoRaw('id="openLiftLink"');
  }

  public function testLinkNotInToolbarInAdminPages() {
    $permissions = [
      'access toolbar',
      'access acquia lift links',
      'access administration pages',
    ];

    // User to set up acquia_lift.
    $linkUser = $this->drupalCreateUser($permissions);
    $this->drupalLogin($linkUser);

    // Check if Acquia Lift Link is available on the node that we created
    $this->drupalGet('/admin');
    // Assert that the Acquia Lift link ID is not present in the HTML.
    $this->assertNoRaw('id="openLiftLink"');

    // Set valid settings
    $this->setValidSettings();
    $this->drupalGet('/admin');
    // Assert that the Acquia Lift link ID is not present in the HTML.
    $this->assertNoRaw('id="openLiftLink"');
  }
}
