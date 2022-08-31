<?php

namespace Drupal\acquia_lift_publisher\EventSubscriber\Cdf;

use Acquia\ContentHubClient\CDF\CDFObject;
use Acquia\ContentHubClient\CDFAttribute;
use Drupal\acquia_contenthub\AcquiaContentHubEvents;
use Drupal\acquia_contenthub\Client\ClientFactory;
use Drupal\acquia_contenthub\Event\CreateCdfEntityEvent;
use Drupal\acquia_contenthub\Session\ContentHubUserSession;
use Drupal\acquia_lift_publisher\Form\ContentPublishingSettingsTrait;
use Drupal\Component\Uuid\UuidInterface;
use Drupal\Core\Block\BlockManagerInterface;
use Drupal\Core\Config\ImmutableConfig;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Language\LanguageManager;
use Drupal\Core\Language\LanguageDefault;
use Drupal\Core\Language\LanguageInterface;
use Drupal\Core\Render\Element;
use Drupal\Core\Render\RendererInterface;
use Drupal\Core\Session\AccountSwitcherInterface;
use Drupal\Core\StringTranslation\TranslationManager;
use Drupal\image\Entity\ImageStyle;
use Symfony\Component\EventDispatcher\EventSubscriberInterface;

/**
 * Entity Renderer Handler Event Subscriber.
 *
 * @package Drupal\acquia_lift_publisher\EventSubscriber\Cdf
 */
class EntityRenderHandler implements EventSubscriberInterface {

  use ContentPublishingSettingsTrait;

  /**
   * The account switcher.
   *
   * @var \Drupal\Core\Session\AccountSwitcherInterface
   */
  protected $accountSwitcher;

  /**
   * The origin uuid.
   *
   * @var string
   */
  protected $origin;

  /**
   * The renderer.
   *
   * @var \Drupal\Core\Render\RendererInterface
   */
  protected $renderer;

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * The Block Manager.
   *
   * @var \Drupal\Core\Block\BlockManagerInterface
   */
  protected $blockManager;

  /**
   * The uuid generator.
   *
   * @var \Drupal\Component\Uuid\UuidInterface
   */
  protected $uuidGenerator;

  /**
   * The rendered user.
   *
   * @var \Drupal\Core\Session\UserSession
   */
  protected $renderUser;

  /**
   * The storage.
   *
   * @var array
   */
  protected $storage = [];

  /**
   * The client factory.
   *
   * @var \Drupal\acquia_contenthub\Client\ClientFactory
   */
  protected $clientFactory;

  /**
   * The language default service.
   *
   * @var \Drupal\Core\Language\LanguageDefault
   */
  protected $languageDefault;

  /**
   * Translation manager.
   *
   * @var \Drupal\Core\StringTranslation\TranslationManager
   */
  protected $translationManager;

  /**
   * EntityRenderHandler constructor.
   *
   * @param \Drupal\Core\Session\AccountSwitcherInterface $account_switcher
   *   The account switcher.
   * @param \Drupal\Core\Config\ImmutableConfig $config
   *   The Acquia Lift publishing configuration object.
   * @param \Drupal\Core\Render\RendererInterface $renderer
   *   The renderer.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager.
   * @param \Drupal\Core\Block\BlockManagerInterface $block_manager
   *   The block manager.
   * @param \Drupal\Component\Uuid\UuidInterface $uuid_generator
   *   The UUID generator.
   * @param \Drupal\acquia_contenthub\Client\ClientFactory $client_factory
   *   The client factory.
   * @param \Drupal\Core\Language\LanguageDefault $language_default
   *   Default Language.
   * @param \Drupal\Core\StringTranslation\TranslationManager $translation_manager
   *   Translation Manager.
   */
  public function __construct(AccountSwitcherInterface $account_switcher, ImmutableConfig $config, RendererInterface $renderer, EntityTypeManagerInterface $entity_type_manager, BlockManagerInterface $block_manager, UuidInterface $uuid_generator, ClientFactory $client_factory, LanguageDefault $language_default, TranslationManager $translation_manager) {
    $this->accountSwitcher = $account_switcher;
    $this->publisherSettings = $config;
    $this->clientFactory = $client_factory;
    $this->origin = $client_factory->getSettings()->getUuid();
    $this->renderer = $renderer;
    $this->entityTypeManager = $entity_type_manager;
    $this->blockManager = $block_manager;
    $this->uuidGenerator = $uuid_generator;
    $this->languageDefault = $language_default;
    $this->translationManager = $translation_manager;
  }

  /**
   * {@inheritdoc}
   */
  public static function getSubscribedEvents() {
    $events[AcquiaContentHubEvents::CREATE_CDF_OBJECT][] = ['onCreateCdf', 100];
    return $events;
  }

  /**
   * Actions on create CDF.
   *
   * @param \Drupal\acquia_contenthub\Event\CreateCdfEntityEvent $event
   *   The create CDF entity event.
   *
   * @throws \Drupal\Component\Plugin\Exception\InvalidPluginDefinitionException
   * @throws \Drupal\Component\Plugin\Exception\PluginNotFoundException
   */
  public function onCreateCdf(CreateCdfEntityEvent $event) {
    $entity = $event->getEntity();
    if (!$entity instanceof ContentEntityInterface) {
      // @todo we should support config entity rendering too.
      return;
    }
    if ($view_modes = $this->getEntityViewModesSettingValue($entity)) {
      $client = $this->clientFactory->getClient();
      if (!is_object($client)) {
        return;
      }
      $document = $client->getEntities([$entity->uuid()]);
      $remote_entity = $document->hasEntity($entity->uuid()) ? $document->getCDFEntity($entity->uuid()) : FALSE;

      $default_lang = $this->languageDefault->get();
      foreach (array_keys($view_modes) as $view_mode) {
        // The preview image field setting is saved along side the view modes.
        // Don't process it as one.
        if ($view_mode == 'acquia_lift_preview_image') {
          continue;
        }
        foreach ($entity->getTranslationLanguages() as $language) {
          $translation = $entity->getTranslation($language->getId());

          if ($remote_entity instanceof CDFObject) {
            $uuid = $this->getRenderUuid($remote_entity, $view_mode, $language->getId());
          }
          else {
            $uuid = $this->uuidGenerator->generate();
          }
          $cdf = new CDFObject('rendered_entity', $uuid, date('c'), date('c'), $this->origin);

          $this->languageDefault->set($language);
          $this->translationManager->setDefaultLangcode($language->getId());
          $standard_languages = LanguageManager::getStandardLanguageList();
          if (isset($standard_languages[$language->getId()][1])) {
            $language_native_label = $standard_languages[$language->getId()][1];
          }
          else {
            $language_native_label = $language->getName();
          }

          // Getting fake user account to give as context to the normalization.
          $account = $this->getRenderUser();

          // Checking for entity access permission to this particular account.
          $entity_access = $translation->access('view', $account, TRUE);

          // Switch to render account.
          $this->accountSwitcher->switchTo($account);
          $elements = [];
          if ($entity_access->isAllowed()) {
            $elements = $this->getViewModeMinimalHtml($translation, $view_mode);
          }
          $html = $this->renderer->renderPlain($elements);

          // Restore user account.
          $this->accountSwitcher->switchBack();

          $metadata['data'] = base64_encode($html);
          $cdf->addAttribute('content', CDFAttribute::TYPE_STRING, trim(preg_replace('/\s+/', ' ', str_replace("\n", ' ', strip_tags($html)))));
          $cdf->addAttribute('source_entity', CDFAttribute::TYPE_STRING, $translation->uuid());
          $cdf->addAttribute('label', CDFAttribute::TYPE_ARRAY_STRING, $translation->label(), $translation->language()->getId());
          $cdf->addAttribute('language', CDFAttribute::TYPE_STRING, $language->getId());
          $cdf->addAttribute('language_label', CDFAttribute::TYPE_STRING, $language_native_label);
          $cdf->addAttribute('view_mode', CDFAttribute::TYPE_STRING, $view_mode);
          $cdf->addAttribute('view_mode_label', CDFAttribute::TYPE_STRING, $view_mode);

          $preview_src = $this->buildPreviewImageAttributeSource($view_modes, 'acquia_lift_preview_image', 'acquia_lift_publisher_preview_image', $translation);

          if (!empty($preview_src)) {
            $cdf->addAttribute('preview_image', CDFAttribute::TYPE_STRING, $preview_src);
          }

          $cdf->setMetadata($metadata);
          $event->addCdf($cdf);
        }
      }

      $this->languageDefault->set($default_lang);
      $this->translationManager->setDefaultLangcode($default_lang->getId());
    }

  }

  /**
   * Build a preview image attribute source.
   *
   * @param array $view_modes
   *   View modes available to check.
   * @param string $preview_view_mode
   *   View mode that represents the preview image.
   * @param string $style
   *   Image style for the preview image.
   * @param \Drupal\Core\Entity\ContentEntityInterface $translation
   *   Translation to use to build the source.
   *
   * @return string|null
   *   The source string returned or nothing.
   */
  protected function buildPreviewImageAttributeSource(array $view_modes, $preview_view_mode, $style, ContentEntityInterface $translation) {

    // Does the view mode exist?
    if (empty($view_modes[$preview_view_mode])) {
      return NULL;
    }

    // Can we load this view mode?
    $preview_image = $translation->{$view_modes[$preview_view_mode]}->first();
    if (empty($preview_image)) {
      return NULL;
    }

    // Can we get a source url for the image style using the view mode?
    $src = ImageStyle::load($style)->buildUrl($preview_image->entity->getFileUri());
    if (empty($src)) {
      return NULL;
    }

    return $src;
  }

  /**
   * Get rendered user.
   *
   * @return \Drupal\acquia_contenthub\Session\ContentHubUserSession|\Drupal\Core\Session\UserSession
   *   The rendered user.
   */
  protected function getRenderUser() {
    if (!$this->renderUser) {
      $this->renderUser = new ContentHubUserSession($this->publisherSettings->get('render_role'));
    }
    return $this->renderUser;
  }

  /**
   * Get view mode minimal HTML.
   *
   * @param \Drupal\Core\Entity\ContentEntityInterface $object
   *   The content entity object.
   * @param string $view_mode
   *   The view mode identifier.
   *
   * @return array
   *   The view mode minimal HTML.
   */
  protected function getViewModeMinimalHtml(ContentEntityInterface $object, $view_mode) {
    // Render View Mode.
    $entity_type_id = $object->getEntityTypeId();
    // @todo allow different entity types to specify how to do this.
    if ($entity_type_id === 'block_content') {
      $build = $this->getBlockMinimalBuildArray($object, $view_mode);
    }
    else {
      $build = $this->getViewMode($object, $view_mode);
    }
    return $build;
  }

  /**
   * Renders block using BlockViewBuilder.
   *
   * @param \Drupal\Core\Entity\ContentEntityInterface $object
   *   The Content Entity Object.
   * @param string $view_mode
   *   The request view mode identifier.
   *
   * @return array
   *   Render array for the block.
   */
  protected function getBlockMinimalBuildArray(ContentEntityInterface $object, $view_mode) {
    /** @var \Drupal\block_content\Plugin\Block\BlockContentBlock $block */
    $block = $this->blockManager->createInstance('block_content:' . $object->uuid());
    $build = [
      '#theme' => 'block',
      '#attributes' => [],
      '#contextual_links' => [],
      '#weight' => 0,
      '#configuration' => $block->getConfiguration(),
      '#plugin_id' => $block->getPluginId(),
      '#base_plugin_id' => $block->getBaseId(),
      '#derivative_plugin_id' => $block->getDerivativeId(),
    ];
    // Label controlled by the block__block_content__acquia_contenthub template
    // (hidden by default). Override the template in your theme to render a
    // block label.
    if ($build['#configuration']['label'] === '') {
      $build['#configuration']['label'] = $object->label();
    }
    // Block entity itself doesn't have configuration.
    $block->setConfigurationValue('view_mode', $view_mode);
    $build['#configuration']['view_mode'] = $view_mode;
    // See \Drupal\block\BlockViewBuilder::preRender() for reference.
    $content = $this->getViewMode($object, $view_mode);
    if ($content !== NULL && !Element::isEmpty($content)) {
      foreach (['#attributes', '#contextual_links'] as $property) {
        if (isset($content[$property])) {
          $build[$property] += $content[$property];
          unset($content[$property]);
        }
      }
    }
    $build['content'] = $content;
    return $build;
  }

  /**
   * Returns the applicable render array.
   *
   * @param \Drupal\Core\Entity\ContentEntityInterface $entity
   *   The renderable entity.
   * @param string $view_mode
   *   The view mode to render in.
   *
   * @return array
   *   The render array.
   */
  protected function getViewMode(ContentEntityInterface $entity, string $view_mode): array {
    return $this->entityTypeManager
      ->getViewBuilder($entity->getEntityTypeId())
      ->view($entity, $view_mode, $entity->language()->getId());
  }

  /**
   * Get rendered content UUID for given source entity view mode and language.
   *
   * A render UUID will be created if one does not already exist in Content Hub
   * for the provided view mode and language.
   *
   * @param \Acquia\ContentHubClient\CDF\CDFObject $source_entity_cdf
   *   The source entity CDF.
   * @param string $view_mode
   *   The view mode identifier.
   * @param string $langcode
   *   The language code.
   *
   * @return mixed
   *   The UUID.
   */
  protected function getRenderUuid(CDFObject $source_entity_cdf, $view_mode, $langcode) {
    $source_entity_uuid = $source_entity_cdf->getUuid();

    if ($this->isStorageHit($source_entity_uuid, $langcode, $view_mode)) {
      return $this->getStorageItem($source_entity_uuid, $langcode, $view_mode);
    }

    // Warm up storage.
    $this->getAllRenderUuids($source_entity_uuid);

    if (!$this->isStorageHit($source_entity_uuid, $langcode, $view_mode)) {
      $this->setStorageItem($source_entity_uuid, $langcode, $view_mode, $this->uuidGenerator->generate());
    }

    return $this->getStorageItem($source_entity_uuid, $langcode, $view_mode);
  }

  /**
   * Get all rendered content UUIDs for a given source entity.
   *
   * @param string $source_entity_uuid
   *   The source entity CDF.
   *
   * @return mixed
   *   The UUIDs.
   */
  protected function getAllRenderUuids($source_entity_uuid) {
    if (!$this->storageIsAlreadyWarmedUp($source_entity_uuid)) {
      $response = $this->clientFactory
        ->getClient()
        ->listEntities([
          'type' => 'rendered_entity',
          'origin' => $this->origin,
          'fields' => 'language,view_mode,source_entity',
          'filters' => [
            'source_entity' => $source_entity_uuid,
          ],
        ]);

      if (TRUE === $response['success'] && !empty($response['data'])) {
        $this->storageWarmUp($response['data'], $source_entity_uuid);
      }
    }

    return $this->getStorageAllItems($source_entity_uuid);
  }

  /**
   * Get attribute value.
   *
   * @param array $attribute
   *   The attribute.
   * @param string $key
   *   The key.
   * @param string|null $default
   *   The default.
   *
   * @return mixed|null
   *   The und value or the default value.
   */
  protected function getAttributeValue(array $attribute, string $key, ?string $default = NULL) {
    return $attribute[$key][LanguageInterface::LANGCODE_NOT_SPECIFIED] ?? $default;
  }

  /**
   * Check if the storage is a hit.
   *
   * @param string $uuid
   *   The UUID.
   * @param string $langcode
   *   The language code.
   * @param string $view_mode
   *   The view mode identifier.
   *
   * @return bool
   *   TRUE if storage hit; FALSE otherwise.
   */
  protected function isStorageHit($uuid, $langcode, $view_mode) {
    return isset($this->storage[$uuid][$langcode][$view_mode]);
  }

  /**
   * Get storage item.
   *
   * @param string $uuid
   *   The UUID.
   * @param string $langcode
   *   The language code.
   * @param string $view_mode
   *   The view mode identifier.
   *
   * @return mixed
   *   The value.
   */
  protected function getStorageItem($uuid, $langcode, $view_mode) {
    return $this->storage[$uuid][$langcode][$view_mode];
  }

  /**
   * Get all items from storage regardless of langcodes and view modes.
   *
   * @param string $uuid
   *   The UUID.
   *
   * @return mixed
   *   The value.
   */
  protected function getStorageAllItems($uuid) {
    return $this->storage[$uuid] ?? [];
  }

  /**
   * Set storage item.
   *
   * @param string $uuid
   *   The UUID.
   * @param string $langcode
   *   The language code.
   * @param string $view_mode
   *   The view mode identifier.
   * @param mixed $value
   *   The value.
   */
  protected function setStorageItem($uuid, $langcode, $view_mode, $value) {
    $this->storage[$uuid][$langcode][$view_mode] = $value;
  }

  /**
   * Check if the storage is already warmed up.
   *
   * @param string $uuid
   *   The UUID.
   *
   * @return bool
   *   TRUE if the storage is already warmed up; FALSE otherwise.
   */
  protected function storageIsAlreadyWarmedUp($uuid) {
    return isset($this->storage[$uuid]);
  }

  /**
   * Storage warm up action.
   *
   * @param array $data
   *   The array of data.
   * @param string $source_uuid
   *   The source UUID.
   */
  protected function storageWarmUp(array $data, $source_uuid) {
    foreach ($data as $entity_info) {
      // Discard any result that does not match exactly the source_uuid.
      if ($this->getAttributeValue($entity_info['attributes'], 'source_entity') === $source_uuid) {
        $this->setStorageItem(
          $source_uuid,
          $this->getAttributeValue($entity_info['attributes'], 'language'),
          $this->getAttributeValue($entity_info['attributes'], 'view_mode'),
          $entity_info['uuid']
        );
      }
    }
  }

}
