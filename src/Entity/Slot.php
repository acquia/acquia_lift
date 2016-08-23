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
 *     "list_builder" = "Drupal\acquia_lift\SlotListBuilder",
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
 *     "html",
 *     "visibility",
 *   },
 *   links = {
 *     "canonical" = "/admin/config/content/acquia-lift/slots",
 *     "add-form" = "/admin/config/content/acquia-lift/slots/add-slot",
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
     * The HTML of the slot identifier.
     *
     * @var string
     */
    protected $html;

    /**
     * Slot Visibility Settings.
     *
     * @var array
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
        return $this->html;
    }

    /**
     * {@inheritdoc}
     */
    public function setHtml($html) {
        $this->html = $html;
        return $this;
    }

    /**
     * {@inheritdoc}
     */
    public function getVisibility() {
        if (!empty($this->visibility) && is_array($this->visibility)) {
            return new Visibility($this->visibility);
        }
        return [];

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
      $slot = new \Acquia\LiftClient\DataObject\Slot();
      $slot->setId($this->id);
      $slot->setLabel($this->label);
      $slot->setDescription($this->description);
      $slot->setStatus($this->status());
      $slot->setHtml($this->html);
      $slot->setVisibility($this->getVisibility());
      return $slot;
    }

}
