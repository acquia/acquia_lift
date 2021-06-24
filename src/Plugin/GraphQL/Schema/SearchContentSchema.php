<?php

namespace Drupal\acquia_perz2\Plugin\GraphQL\Schema;

use Drupal\graphql\GraphQL\ResolverBuilder;
use Drupal\graphql\GraphQL\ResolverRegistry;
use Drupal\graphql\Plugin\GraphQL\Schema\SdlSchemaPluginBase;
use Drupal\ac_graphql_test\Wrappers\QueryConnection;

/**
 * @Schema(
 *   id = "search_content",
 *   name = "Search Content schema"
 * )
 */
class SearchContentSchema extends SdlSchemaPluginBase {

  /**
   * {@inheritdoc}
   */
  public function getResolverRegistry() {
    $builder = new ResolverBuilder();
    $registry = new ResolverRegistry();

    $this->addQueryFields($registry, $builder);
    $this->addEntityFields($registry, $builder);

    return $registry;
  }

  /**
   * @param \Drupal\graphql\GraphQL\ResolverRegistry $registry
   * @param \Drupal\graphql\GraphQL\ResolverBuilder $builder
   */
  protected function addEntityFields(ResolverRegistry $registry, ResolverBuilder $builder) {
    $registry->addFieldResolver('Entity', 'id',
      $builder->produce('entity_uuid')
        ->map('entity', $builder->fromParent())
    );

    $registry->addFieldResolver('Entity', 'title',
      $builder->produce('entity_label')
        ->map('entity', $builder->fromParent())
    );

    $registry->addFieldResolver('Entity', 'created',
      $builder->produce('entity_created')
        ->map('entity', $builder->fromParent())
    );

    $registry->addFieldResolver('Entity', 'updated',
      $builder->produce('entity_changed')
        ->map('entity', $builder->fromParent())
    );

    $registry->addFieldResolver('Entity', 'base_url',
      $builder->produce('base_url')
    );

    $registry->addFieldResolver('Entity', 'view_mode',
      $builder->produce('single_view_mode')
        ->map('entity', $builder->fromParent())
        ->map('view_mode', $builder->fromArgument('view_mode'))
        ->map('context_language', $builder->fromArgument('context_language'))
    );
  }

  /**
   * @param \Drupal\graphql\GraphQL\ResolverRegistry $registry
   * @param \Drupal\graphql\GraphQL\ResolverBuilder $builder
   */
  protected function addQueryFields(ResolverRegistry $registry, ResolverBuilder $builder) {
    ////////
    $registry->addFieldResolver('Query', 'entity_single_mode',
      $builder->produce('entity_load')
        ->map('type', $builder->fromArgument('entityType'))
        ->map('id', $builder->fromArgument('id'))
        ->map('language', $builder->fromArgument('context_language'))
    );
    $registry->addFieldResolver('Query', 'search_content',
      $builder->compose(
        $builder->produce('query_entities')
          ->map('entity_type_id', $builder->fromArgument('content_type'))
          ->map('start', $builder->fromArgument('start'))
          ->map('rows', $builder->fromArgument('rows'))
          ->map('langcode', $builder->fromArgument('context_language'))
          ->map('date_start', $builder->fromArgument('date_start'))
          ->map('date_end', $builder->fromArgument('date_end'))
          ->map('q', $builder->fromArgument('q'))
          ->map('tags', $builder->fromArgument('tags'))
          ->map('all_tags', $builder->fromArgument('all_tags'))
          ->map('sort', $builder->fromArgument('sort'))
          ->map('sort_order', $builder->fromArgument('sort_order'))
          ->map('exclude', $builder->fromArgument('exclude')),
        $builder->produce('entity_load_multiple')
          ->map('type', $builder->fromValue('node'))
          ->map('ids', $builder->fromParent())
          ->map('language', $builder->fromArgument('context_language'))
      )
    );
  }

}
