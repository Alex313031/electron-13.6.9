// Copyright (c) 2017 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/mac/in_app_purchase_product.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

#import <StoreKit/StoreKit.h>

// ============================================================================
//                             InAppPurchaseProduct
// ============================================================================

// --------------------------------- Interface --------------------------------

@interface InAppPurchaseProduct : NSObject <SKProductsRequestDelegate> {
 @private
  in_app_purchase::InAppPurchaseProductsCallback callback_;
}

- (id)initWithCallback:(in_app_purchase::InAppPurchaseProductsCallback)callback;

@end

// ------------------------------- Implementation -----------------------------

@implementation InAppPurchaseProduct

/**
 * Init with a callback.
 *
 * @param callback - The callback that will be called to return the products.
 */
- (id)initWithCallback:
    (in_app_purchase::InAppPurchaseProductsCallback)callback {
  if ((self = [super init])) {
    callback_ = std::move(callback);
  }

  return self;
}

/**
 * Return products.
 *
 * @param productIDs - The products' id to fetch.
 */
- (void)getProducts:(NSSet*)productIDs {
  SKProductsRequest* productsRequest;
  productsRequest =
      [[SKProductsRequest alloc] initWithProductIdentifiers:productIDs];

  productsRequest.delegate = self;
  [productsRequest start];
}

/**
 * @see SKProductsRequestDelegate
 */
- (void)productsRequest:(SKProductsRequest*)request
     didReceiveResponse:(SKProductsResponse*)response {
  // Release request object.
  [request release];

  // Get the products.
  NSArray* products = response.products;

  // Convert the products.
  std::vector<in_app_purchase::Product> converted;
  converted.reserve([products count]);

  for (SKProduct* product in products) {
    converted.push_back([self skProductToStruct:product]);
  }

  // Send the callback to the browser thread.
  base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                 base::BindOnce(std::move(callback_), converted));

  [self release];
}

/**
 * Format local price.
 *
 * @param price - The price to format.
 * @param priceLocal - The local format.
 */
- (NSString*)formatPrice:(NSDecimalNumber*)price
               withLocal:(NSLocale*)priceLocal {
  NSNumberFormatter* numberFormatter = [[NSNumberFormatter alloc] init];

  [numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
  [numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
  [numberFormatter setLocale:priceLocal];

  return [numberFormatter stringFromNumber:price];
}

/**
 * Convert a skProduct object to Product structure.
 *
 * @param product - The SKProduct object to convert.
 */
- (in_app_purchase::Product)skProductToStruct:(SKProduct*)product {
  in_app_purchase::Product productStruct;

  // Product Identifier
  if (product.productIdentifier != nil) {
    productStruct.productIdentifier = [product.productIdentifier UTF8String];
  }

  // Product Attributes
  if (product.localizedDescription != nil) {
    productStruct.localizedDescription =
        [product.localizedDescription UTF8String];
  }
  if (product.localizedTitle != nil) {
    productStruct.localizedTitle = [product.localizedTitle UTF8String];
  }
  if (product.contentVersion != nil) {
    productStruct.contentVersion = [product.contentVersion UTF8String];
  }
  if (product.contentLengths != nil) {
    productStruct.contentLengths.reserve([product.contentLengths count]);

    for (NSNumber* contentLength in product.contentLengths) {
      productStruct.contentLengths.push_back([contentLength longLongValue]);
    }
  }

  // Pricing Information
  if (product.price != nil) {
    productStruct.price = [product.price doubleValue];

    if (product.priceLocale != nil) {
      productStruct.formattedPrice =
          [[self formatPrice:product.price
                   withLocal:product.priceLocale] UTF8String];

      // Currency Information
      if (@available(macOS 10.12, *)) {
        if (product.priceLocale.currencyCode != nil) {
          productStruct.currencyCode =
              [product.priceLocale.currencyCode UTF8String];
        }
      }
    }
  }

  // Downloadable Content Information
  productStruct.isDownloadable = [product downloadable];

  return productStruct;
}

@end

// ============================================================================
//                             C++ in_app_purchase
// ============================================================================

namespace in_app_purchase {

Product::Product() = default;
Product::Product(const Product&) = default;
Product::~Product() = default;

void GetProducts(const std::vector<std::string>& productIDs,
                 InAppPurchaseProductsCallback callback) {
  auto* iapProduct =
      [[InAppPurchaseProduct alloc] initWithCallback:std::move(callback)];

  // Convert the products' id to NSSet.
  NSMutableSet* productsIDSet =
      [NSMutableSet setWithCapacity:productIDs.size()];

  for (auto& productID : productIDs) {
    [productsIDSet addObject:base::SysUTF8ToNSString(productID)];
  }

  // Fetch the products.
  [iapProduct getProducts:productsIDSet];
}

}  // namespace in_app_purchase
