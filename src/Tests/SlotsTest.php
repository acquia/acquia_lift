<?php

namespace Drupal\acquia_lift\Tests;

use Drupal\simpletest\WebTestBase;

/**
 * Test Acquia Lift Slots.
 *
 * @group Acquia Lift
 */
class SlotsTest extends WebTestBase {

  use FixturesDataTrait;

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = ['node', 'taxonomy', 'acquia_lift'];

  /**
   * {@inheritdoc}
   */
  protected function setUp() {
    parent::setUp();

    $permissions = [
      'access administration pages',
      'administer acquia_lift'
    ];

    // User to set up acquia_lift.
    $this->admin_user = $this->drupalCreateUser($permissions);
    $this->drupalLogin($this->admin_user);
  }

  public function testAdminSlotForm() {
    // Check for setting page's presence.
    $this->drupalGet('admin/config/content/acquia-lift/slot');
    $this->assertRaw(t('Slots'), '[testAdminSlotsForm]: Slots page displayed.');
  }

  /*public function testSlotAdd() {
    // Check for setting page's presence.
    $this->drupalGet('admin/config/content/acquia-lift/slot');
    $this->assertRaw(t('Slots'), '[testAdminSlotsForm]: Slots page displayed.');
  }

  public function testSlotEdit() {
    // Check for setting page's presence.
    $this->drupalGet('admin/config/content/acquia-lift/slot');
    $this->assertRaw(t('Slots'), '[testAdminSlotsForm]: Slots page displayed.');
  }

  public function testSlotDelete() {
    // Check for setting page's presence.
    $this->drupalGet('admin/config/content/acquia-lift/slot');
    $this->assertRaw(t('Slots'), '[testAdminSlotsForm]: Slots page displayed.');
  }

  public function testSlotDisable() {
    // Check for setting page's presence.
    $this->drupalGet('admin/config/content/acquia-lift/slot');
    $this->assertRaw(t('Slots'), '[testAdminSlotsForm]: Slots page displayed.');
  }*/

}
