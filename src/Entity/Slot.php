<?php

namespace Drupal\acquia_lift\Entity;

use Acquia\LiftClient\Entity\Visibility;
use Drupal\acquia_lift\AcquiaLiftException;
use Drupal\acquia_lift\SlotInterface;
use Drupal\Core\Config\Entity\ConfigEntityBase;
use Drupal\Core\Entity\EntityStorageInterface;

/**
 * Defines the Drupal created slots.
 *
 * @ConfigEntityType(
 *   id = "acquia_lift_slot",
 *   label = @Translation("Slot"),
 *   label_singular = @Translation("slot"),
 *   label_plural = @Translation("slots"),
 *   label_count = @PluralTranslation(
 *     singular = "@count slot",
 *     plural = "@count slots",
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
 *   },
 *   links = {
 *     "add-form" = "/admin/config/content/acquia-lift/slots/add-slot",
 *     "edit-form" = "/admin/config/content/acquia-lift/slots/{acquia_lift_slot}",
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
   * The Lift API Helper.
   *
   * @var \Acquia\LiftClient\Lift
   */
  protected $liftClient;

  /**
   * {@inheritdoc}
   */
  public function __construct(array $values, $entity_type) {
    parent::__construct($values, $entity_type);

    try {
      /** @var \Drupal\acquia_lift\Service\Helper\LiftAPIHelper $liftHelper */
      $liftHelper = \Drupal::getContainer()
        ->get('acquia_lift.service.helper.lift_api_helper');
      $this->liftClient = $liftHelper->getLiftClient();
    } catch (AcquiaLiftException $e) {
      drupal_set_message(t($e->getMessage()), 'error');
    }
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
  public function getHtml() {
    return "<div data-lift-slot=\"" . $this->uuid() . "\"/>";
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
    $slot = new \Acquia\LiftClient\Entity\Slot();
    $slot->setId($this->uuid());
    $slot->setLabel($this->label);
    $slot->setDescription($this->description);
    $slot->setStatus($this->status());
    $slot->setHtml($this->getHtml());
    $slot->setVisibility($this->getVisibility());
    return $slot;
  }

  /**
   * {@inheritdoc}
   */
  public function save() {
    $return = parent::save();
    try {
      $this->liftClient->getSlotManager()->add($this->getExternalSlot());
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

    foreach ($entities as $entity) {
      if ($entity->isUninstalling() || $entity->isSyncing()) {
        // During extension uninstall and configuration synchronization
        // deletions are already managed.
        break;
      }
      try {
        $entity->liftClient->getSlotManager()->delete($entity->uuid());
      } catch (\Exception $e) {
        drupal_set_message(t($e->getMessage()), 'error');
      }
    }
  }
}
