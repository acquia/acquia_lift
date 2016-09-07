<?php

namespace Drupal\acquia_lift\Tests;

use Drupal\acquia_lift\Entity\Slot;
use Drupal\Component\Utility\Html;
use Drupal\simpletest\WebTestBase;

/**
 * Test Acquia Lift Slots.
 *
 * @group Acquia Lift
 */
class SlotsTest extends WebTestBase {

  use SettingsDataTrait;

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = ['node', 'taxonomy', 'acquia_lift'];

  /**
   * The Slot's id used in this test.
   *
   * @var string
   */
  public $slotId;

  /**
   * The Slot's label used in this test.
   *
   * @var string
   */
  public $slotLabel;

  /**
   * The Slot's description used in this test.
   *
   * @var string
   */
  public $slotDescription;

  public $slotVisibility;

  /**
   * The admin user.
   *
   * @var \Drupal\user\Entity\User
   */
  public $adminUser;

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
    $this->adminUser = $this->drupalCreateUser($permissions);
    $this->drupalLogin($this->adminUser);

    $this->slotId = 'test_slot_id';
    $this->slotLabel = 'test slot label';
    $this->slotDescription = 'test slot description';
    $this->slotVisibility = 'localhost';
  }

  public function testSlotListingsPage() {
    // Check for slot listings page's presence.
    $this->drupalGet('admin/config/content/acquia-lift/slot');
    $this->assertRaw(t('Slots'), '[testAdminSlotsForm]: Slots page displayed.');
  }

  /**
   * Test if we can do CRUD actions to slots without having service keys setup
   * in Acquia Lift.
   */
  public function testSlotCrudWithoutServiceCreds() {
    // Create the slot  and verify if it succeeded
    $this->slotCreate();
    // Check if it appears in the list and is unable to authenticate.
    $this->checkIfSlotIsDisabledDueToMissingCredentials();

    // Modify the name and verify if it succeeded
    $this->slotModifyName();

    // Disable the slot and verify if it succeeded
    $this->slotDisable();

    // Check if it appears in the list and is unable to authenticate.
    $this->checkIfSlotIsDisabledDueToMissingCredentials();

    // Delete the slot and verify if it succeeded
    $this->slotDelete();
  }

  /**
   * Test if we can do CRUD actions to slots without having service keys setup
   * in Acquia Lift.
   */
  public function testSlotCrudWithServiceCreds() {
    // Enter fake credentials to test the different behavior
    $this->setupAcquiaLiftSettings();

    // Create the slot and verify if it succeeded
    $this->slotCreate();

    // Check if it appears in the list and is unable to authenticate.
    $this->checkIfSlotIsDisabledDueToBadResponse();

    // Disable the slot and verify if it succeeded
    $this->slotDisable();

    // Check if it appears in the list and is unable to authenticate.
    $this->checkIfSlotIsDisabledDueToBadResponse();

    // Delete the slot and verify if it succeeded
    $this->slotDelete();
  }

  public function setupAcquiaLiftSettings() {
    // Check for setting page's presence.
    $this->drupalGet('admin/config/content/acquia-lift');
    $this->assertRaw(t('Acquia Lift settings'), '[testAdminSettingsForm]: Settings page displayed.');

    // Get all the valid settings, and massage them into form $edit array.
    $credential_settings = $this->getValidCredentialSettings();

    $edit = [];
    $edit += $this->convertToPostFormSettings($credential_settings, 'credential');

    // Post the edits and assert that options are saved.
    $this->drupalPostForm('admin/config/content/acquia-lift', $edit, t('Save configuration'));
    $this->assertText(t('The configuration options have been saved.'));

  }

  public function slotCreate() {
    $this->drupalGet('admin/config/content/acquia-lift/slot/add');
    // Get all the slot properties settings, and massage them into form $edit array.
    $edit = [];
    $edit['id'] = $this->slotId;
    $edit['label'] = $this->slotLabel;
    $edit['description'] = $this->slotDescription;
    $edit['pages'] = $this->slotVisibility;

    // Post the edits and assert that options are saved.
    $this->drupalPostForm('admin/config/content/acquia-lift/slot/add', $edit, t('Save'));
    $this->assertText(t('Slot @slot has been added.', array('@slot' => $this->slotLabel)));

    $slot = $this->getTestSlot();

    // Check if the listing is correct
    $this->drupalGet('admin/config/content/acquia-lift/slot');

    // Check if the label is the expected label
    $this->assertText($slot->label(), 'Slot present on overview page.');
    $this->assertRaw($slot->getDescription(), 'Description is present on overview page.');

    // Check if the HTML is correct
    $this->assertRaw(Html::escape($slot->getHtml()), 'HTML is present on overview page');
  }

  public function checkIfSlotIsDisabledDueToMissingCredentials() {
    $slot = $this->getTestSlot();
    // Check if the slot has the right properties. It should be disabled as we do not have any service setup.
    $this->assertFieldByXPath('//tr[contains(@class,"' . Html::cleanCssIdentifier($slot->getEntityTypeId() . '-' . $slot->id()) . '") and contains(@class, "acquia-lift-slot-disabled")]', NULL, 'Slot is shown as disabled');

    // Check if the icon used is the error icon and the error message is unauthenticated.
    $this->assertFieldByXPath('//td[@class="checkbox" and ./img[@src="/core/misc/icons/e32700/error.svg" and @title="One or more required authorization header fields (ID, nonce, realm, version) are missing."]]', NULL, 'Slot has the error status and icon.');
  }

  public function checkIfSlotIsDisabledDueToBadResponse() {
    $slot = $this->getTestSlot();
    // Check if the slot has the right properties. It should be disabled as we do not have any service setup.
    $this->assertFieldByXPath('//tr[contains(@class,"' . Html::cleanCssIdentifier($slot->getEntityTypeId() . '-' . $slot->id()) . '") and contains(@class, "acquia-lift-slot-disabled")]', NULL, 'Slot is shown as disabled');

    // Check if the icon used is the error icon and the error message is unauthenticated.
    $this->assertFieldByXPath('//td[@class="checkbox" and ./img[@src="/core/misc/icons/e32700/error.svg" and @title="Response is missing required X-Server-Authorization-HMAC-SHA256 header."]]', NULL, 'Slot has the error status and icon.');
  }

  public function slotModifyName() {
    // Check for setting page's presence.
    $this->drupalGet('admin/config/content/acquia-lift/slot/' . $this->slotId . '/edit');

    // Get all the slot properties settings, and massage them into form $edit array.
    $edit = [];
    $edit['label'] = $this->slotLabel . '-updated';

    // Post the edits and assert that options are saved.
    $this->drupalPostForm(NULL, $edit, t('Save'));

    // Reload the slot
    $slot = $this->getTestSlot();

    $this->assertText(t('Slot @slot has been updated.', array('@slot' => $slot->label())));

    // Check if the listing is correct
    $this->drupalGet('admin/config/content/acquia-lift/slot');

    // Check if the label is the expected label
    $this->assertText($slot->label(), 'Slot present on overview page.');
    $this->assertRaw($slot->getDescription(), 'Description is present on overview page.');

    // Check if the HTML is correct
    $this->assertRaw(Html::escape($slot->getHtml()), 'HTML is present on overview page');
  }

  public function slotDisable() {
    // Check for setting page's presence.
    $this->drupalGet('admin/config/content/acquia-lift/slot');
    $this->assertRaw(t('Slots'), '[testAdminSlotsForm]: Slots page displayed.');
    $this->clickLink('Disable');
    // Submit the confirmation form and test that the entity has been disabled.
    $this->drupalPostForm(NULL, array(), 'Disable');
  }

  public function slotDelete() {
    $slot = $this->getTestSlot();
    // Check for setting page's presence.
    $this->drupalGet('admin/config/content/acquia-lift/slot');
    $this->assertRaw(t('Slots'), '[testAdminSlotsForm]: Slots page displayed.');
    $this->clickLink('Delete');
    // Submit the confirmation form and test that the entity has been disabled.
    $this->drupalPostForm(NULL, array(), 'Delete');

    // Check if it is gone from the list.
    $this->drupalGet('admin/config/content/acquia-lift/slot');
    $this->assertNoText($slot->label(), 'Slot is present on overview page.');
    $this->assertNoRaw($slot->getDescription(), 'Description is present on overview page.');
  }

  /**
   * Creates or loads a slot.
   *
   * @return \Drupal\acquia_lift\SlotInterface
   *   An acquia lift slot.
   */
  public function getTestSlot() {
    $slot = Slot::load($this->slotId);
    if (!$slot) {
      $slot = Slot::create(array(
        'id' => $this->slotId,
        'name' => $this->slotLabel,
        'description' => $this->slotDescription,
        'visibility' => array($this->slotVisibility),
      ));
      $slot->save();
    }

    return $slot;
  }

}
