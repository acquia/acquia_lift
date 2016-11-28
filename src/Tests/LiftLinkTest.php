<?php

namespace Drupal\acquia_lift\Tests;

use Drupal\simpletest\WebTestBase;

/**
 * Test Acquia Lift Toolbar Link.
 *
 * @group Acquia Lift
 */
class LiftLinkTest extends WebTestBase {

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

    $permissions = [
      'access toolbar',
      'access acquia lift links',
    ];

    // User to set up acquia_lift.
    $linkUser = $this->drupalCreateUser($permissions);
    $this->drupalLogin($linkUser);
  }

  public function testLinkInToolbar() {
    // Check if Acquia Lift Link is available on the frontpage in the toolbar.
    $this->drupalGet('<front>');
    // Assert that the Acquia Lift link ID is present in the HTML.
    $this->assertRaw('id="openLiftLink"');
  }
}
