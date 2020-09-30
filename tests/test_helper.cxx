
#include <gtest/gtest.h>

#include "gtensor/gtensor.h"
#include "gtensor/helper.h"

#include <tuple>
#include <type_traits>

#include "test_debug.h"

TEST(helper, tuple_max)
{
  std::tuple<> t0;
  std::tuple<int> t1 = {5};
  std::tuple<int, int> t2 = {5, 10};
  std::tuple<int, int, int> t3 = {5, 10, 15};
  std::tuple<int, int, int> t3a = {25, 10, 15};

  auto id = [](auto& val) { return val; };
  EXPECT_EQ(gt::helper::max(id, t0), 0);
  EXPECT_EQ(gt::helper::max(id, t1), 5);
  EXPECT_EQ(gt::helper::max(id, t2), 10);
  EXPECT_EQ(gt::helper::max(id, t3), 15);
  EXPECT_EQ(gt::helper::max(id, t3a), 25);
}

TEST(helper, nd_initializer_list)
{
  using namespace gt::helper;

  nd_initializer_list_t<int, 1> nd1 = {1, 2, 3, 4, 5, 6};
  auto nd1shape = nd_initializer_list_shape<1>(nd1);
  EXPECT_EQ(nd1shape, gt::shape(6));

  nd_initializer_list_t<int, 2> nd2 = {{1, 2, 3}, {4, 5, 6}};
  auto nd2shape = nd_initializer_list_shape<2>(nd2);
  EXPECT_EQ(nd2shape, gt::shape(3, 2));

  nd_initializer_list_t<int, 3> nd3 = {{{
                                          1,
                                        },
                                        {
                                          2,
                                        },
                                        {
                                          3,
                                        }},
                                       {{
                                          4,
                                        },
                                        {
                                          5,
                                        },
                                        {
                                          6,
                                        }}};
  auto nd3shape = nd_initializer_list_shape<3>(nd3);
  EXPECT_EQ(nd3shape, gt::shape(1, 3, 2));
}

TEST(helper, to_expression_t)
{
  gt::gtensor<double, 1> a;
  auto a_view = a.view();
  using to_expr_view_type = gt::to_expression_t<decltype(a_view)>;

  GT_DEBUG_TYPE(a);
  GT_DEBUG_TYPE(a_view);
  GT_DEBUG_TYPE_NAME(to_expr_view_type);

  EXPECT_TRUE((std::is_same<decltype(a_view), to_expr_view_type>::value));
}

TEST(helper, const_view_to_expression_t)
{
  gt::gtensor<double, 1> a;
  const auto a_view = a.view();
  using to_expr_view_type = gt::to_expression_t<decltype(a_view)>;

  GT_DEBUG_TYPE(a);
  GT_DEBUG_TYPE(a_view);
  GT_DEBUG_TYPE_NAME(to_expr_view_type);

  EXPECT_TRUE((std::is_same<decltype(a_view), to_expr_view_type>::value));
}

TEST(helper, const_gtensor_to_expression_t)
{
  const gt::gtensor<double, 1> a;
  auto a_view = a.view();
  using to_expr_view_type = gt::to_expression_t<decltype(a_view)>;

  GT_DEBUG_TYPE(a);
  GT_DEBUG_TYPE(a_view);
  GT_DEBUG_TYPE_NAME(to_expr_view_type);

  EXPECT_TRUE((std::is_same<decltype(a_view), to_expr_view_type>::value));
}

TEST(helper, gtensor_to_kernel_t)
{
  gt::gtensor<double, 1> a;
  auto k_a = a.to_kernel();
  using to_kern_type = gt::to_kernel_t<decltype(a)>;

  GT_DEBUG_TYPE(a);
  GT_DEBUG_TYPE(k_a);
  GT_DEBUG_TYPE_NAME(to_kern_type);

  EXPECT_TRUE((std::is_same<decltype(k_a), to_kern_type>::value));
}

TEST(helper, const_gtensor_to_kernel_t)
{
  const gt::gtensor<double, 1> a;
  auto k_a = a.to_kernel();
  using to_kern_type = gt::to_kernel_t<decltype(a)>;

  GT_DEBUG_TYPE(a);
  GT_DEBUG_TYPE(k_a);
  GT_DEBUG_TYPE_NAME(to_kern_type);

  EXPECT_TRUE((std::is_same<decltype(k_a), to_kern_type>::value));
}

TEST(helper, view_to_kernel_t)
{
  gt::gtensor<double, 1> a;
  auto a_view = a.view();
  auto k_view = a_view.to_kernel();
  using to_kern_view_type = gt::to_kernel_t<decltype(a_view)>;

  GT_DEBUG_TYPE(a);
  GT_DEBUG_TYPE(a_view);
  GT_DEBUG_TYPE(k_view);
  GT_DEBUG_TYPE_NAME(to_kern_view_type);

  EXPECT_TRUE((std::is_same<decltype(k_view), to_kern_view_type>::value));
}

TEST(helper, view_const_o_kernel_t)
{
  gt::gtensor<double, 1> a;
  const auto a_view = a.view();
  auto k_view = a_view.to_kernel();
  using to_kern_view_type = gt::to_kernel_t<decltype(a_view)>;

  GT_DEBUG_TYPE(a);
  GT_DEBUG_TYPE(a_view);
  GT_DEBUG_TYPE(k_view);
  GT_DEBUG_TYPE_NAME(to_kern_view_type);

  EXPECT_TRUE((std::is_same<decltype(k_view), to_kern_view_type>::value));
}

TEST(helper, view_gtensor_const_to_kernel_t)
{
  const gt::gtensor<double, 1> a;
  auto a_view = a.view();
  auto k_view = a_view.to_kernel();
  using to_kern_view_type = gt::to_kernel_t<decltype(a_view)>;

  GT_DEBUG_TYPE(a);
  GT_DEBUG_TYPE(a_view);
  GT_DEBUG_TYPE(k_view);
  GT_DEBUG_TYPE_NAME(to_kern_view_type);

  EXPECT_TRUE((std::is_same<decltype(k_view), to_kern_view_type>::value));
}
