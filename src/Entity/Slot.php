<?php

namespace Drupal\acquia_lift\Entity;

use Acquia\LiftClient\Entity\Visibility;
use Drupal\acquia_lift\SlotInterface;
use Drupal\Core\Config\Entity\ConfigEntityBase;
use Drupal\Core\Entity\EntityStorageInterface;
use Acquia\LiftClient\Entity\Slot as LiftClientSlot;

/**
 * Defines the Drupal created slots.
 *
 * @ConfigEntityType(
 *   id = "acquia_lift_slot",
 *   label = @Translation("Slot"),
 *   label_singular = @Translation("Slot"),
 *   label_plural = @Translation("Slots"),
 *   label_count = @PluralTranslation(
 *     singular = "@count Slot",
 *     plural = "@count Slots",
 *   ),
 *   handlers = {
 *     "storage" = "Drupal\Core\Config\Entity\ConfigEntityStorage",
 *     "list_builder" = "Drupal\acquia_lift\Entity\Controller\SlotListBuilder",
 *     "form" = {
 *       "default" = "Drupal\acquia_lift\Form\SlotForm",
 *       "edit" = "Drupal\acquia_lift\Form\SlotForm",
 *       "delete" = "Drupal\acquia_lift\Form\SlotDeleteConfirmForm",
 *       "disable" = "Drupal\acquia_lift\Form\SlotDisableConfirmForm",
 *       "enable" = "Drupal\acquia_lift\Form\SlotEnableConfirmForm",
 *     },
 *   },
 *   admin_permission = "administer acquia_lift",
 *   config_prefix = "slot",
 *   entity_keys = {
 *     "id" = "id",
 *     "label" = "label",
 *     "uuid" = "uuid",
 *     "status" = "status",
 *   },
 *   config_export = {
 *     "id",
 *     "label",
 *     "description",
 *     "visibility",
 *     "css_url"
 *   },
 *   links = {
 *     "add-form" = "/admin/config/content/acquia-lift/slots/add",
 *     "edit-form" = "/admin/config/content/acquia-lift/slots/{acquia_lift_slot}/edit",
 *     "delete-form" = "/admin/config/content/acquia-lift/slots/{acquia_lift_slot}/delete",
 *     "disable" = "/admin/config/content/acquia-lift/slots/{acquia_lift_slot}/disable",
 *     "enable" = "/admin/config/content/acquia-lift/slots/{acquia_lift_slot}/enable",
 *   }
 * )
 */
class Slot extends ConfigEntityBase implements SlotInterface {

  /**
   * The ID of the slot.
   *
   * @var string
   */
  protected $id;

  /**
   * The slot label.
   *
   * @var string
   */
  protected $label;

  /**
   * A string describing the slot.
   *
   * @var string
   */
  protected $description;

  /**
   * The slot visibility settings
   *
   * @var string
   */
  protected $visibility;

  /**
   * The slot CSS Url
   *
   * It has the underscore as Drupal does this auto-mapping of properties to
   * config schema. If anyone knows how we can use camelCase here, it's be great
   * but it's not a huge deal.
   *
   * @var string
   */
  protected $css_url;

  /**
   * {@inheritdoc}
   */
  public function __construct(array $values, $entity_type) {
    parent::__construct($values, $entity_type);
  }

  /**
   * {@inheritdoc}
   */
  public function getDescription() {
    return $this->description;
  }

  /**
   * {@inheritdoc}
   */
  public function setDescription($description) {
    $this->description = $description;
    return $this;
  }

  /**
   * {@inheritdoc}
   */
  public function getCssUrl() {
    return $this->css_url;
  }

  /**
   * {@inheritdoc}
   */
  public function setCssUrl($cssUrl) {
    $this->css_url = $cssUrl;
    return $this;
  }

  /**
   * {@inheritdoc}
   */
  public function getHtml() {
    $cssHtml = "";
    if (!empty($this->css_url)) {
      $cssHtml = " data-lift-css=\"" . $this->css_url . "\"";
    }
    return "<div data-lift-slot=\"" . $this->id() . "\"" . $cssHtml . "/>";
  }

  /**
   * {@inheritdoc}
   */
  public function getVisibility() {
    if (!empty($this->visibility) && is_array($this->visibility)) {
      return new Visibility($this->visibility);
    }
    return new Visibility([]);
  }

  /**
   * {@inheritdoc}
   */
  public function setVisibility(Visibility $visibility) {
    $this->visibility = $visibility->getArrayCopy();
    return $this;
  }

  /**
   * {@inheritdoc}
   */
  public function getExternalSlot() {
    $slot = new LiftClientSlot();
    $slot->setId($this->id());
    $slot->setLabel($this->label);
    $slot->setDescription($this->description);
    $slot->setStatus($this->status());
    $slot->setHtml($this->getHtml());
    $slot->setVisibility($this->getVisibility());
    return $slot;
  }

  /**
   * Returns the APILoader class.
   *
   * Drupal 8 does not support dependency injection in Config Entities, it does
   * support dependency injection in the Events using the EventSubscriber but
   * there are no events available for entity save and entity delete. We could
   * use hooks but that leaves us with the same problem and is not any better
   * than loading the service here as a static function where it is being used.
   *
   * @return \Drupal\acquia_lift\Lift\APILoader
   */
  public static function getLiftAPILoader() {
    return \Drupal::service('acquia_lift.lift.api_loader');
  }

  /**
   * {@inheritdoc}
   */
  public function save() {
    $return = parent::save();
    // Try saving it in the decision api and show any error in the UI if it
    // would fail.
    try {
      Slot::getLiftAPILoader()
        ->getLiftClient()
        ->getSlotManager()
        ->add($this->getExternalSlot());
    } catch (\Exception $e) {
      drupal_set_message(t($e->getMessage()), 'error');
    }
    return $return;
  }

  /**
   * {@inheritdoc}
   */
  public static function preDelete(EntityStorageInterface $storage, array $entities) {
    parent::preDelete($storage, $entities);

    /** @var \Drupal\acquia_lift\Entity\Slot $entity */
    foreach ($entities as $entity) {
      if ($entity->isUninstalling() || $entity->isSyncing()) {
        // During extension uninstall and configuration synchronization
        // deletions are already managed.
        break;
      }
      // Try deleting it in the decision api and show any error in the UI if it
      // would fail.
      try {
        Slot::getLiftAPILoader()
          ->getLiftClient()->getSlotManager()->delete($entity->id());
      } catch (\Exception $e) {
        drupal_set_message(t($e->getMessage()), 'error');
      }
    }
  }
}
