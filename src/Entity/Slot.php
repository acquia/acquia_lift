<?php

namespace Drupal\acquia_lift\Entity;

use Acquia\LiftClient\DataObject\Visibility;
use Drupal\acquia_lift\SlotInterface;
use Drupal\Core\Config\Entity\ConfigEntityBase;

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

}
