<?php

namespace Drupal\acquia_lift;

use \Acquia\LiftClient\DataObject\Slot;
use \Acquia\LiftClient\DataObject\Visibility;
use Drupal\Core\Config\Entity\ConfigEntityInterface;

interface SlotInterface extends ConfigEntityInterface
{

    /**
     * Get the Slot Description.
     *
     * @return string
     *   The Slot description.
     */
    public function getDescription();

    /**
     * Set the slot description.
     *
     * @param string $description
     *   The Slot Description.
     *
     * @return $this
     *   Returns self.
     */
    public function setDescription($description);

    /**
     * Get the Slot's HTML.
     *
     * @return string
     *   The slot's html.
     */
    public function getHtml();

    /**
     * Set the slot's HTMl.
     *
     * @param string $html
     *   The Slot's HTML
     *
     * @return $this
     *   Returns self.
     */
    public function setHtml($html);

    /**
     * Get the Slot's visibility settings
     *
     * @return \Acquia\LiftClient\DataObject\Visibility
     *   The slot's visibility.
     */
    public function getVisibility();

    /**
     * Set the Slot's visibility settings.
     *
     * @param \Acquia\LiftClient\DataObject\Visibility $visibility
     *   The slot's visibility settings.
     *
     * @return $this
     *   Returns self.
     */
    public function setVisibility(Visibility $visibility);

    /**
     * Get the Config Entity as a Acquia Lift Slot object.
     *
     * @return \Acquia\LiftClient\DataObject\Slot
     *   The Acquia Lift Slot object.
     */
    public function getExternalSlot();

}