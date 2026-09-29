/*
 * Copyright (c) 2024 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/video/video.h>
#include <zephyr/ztest.h>

enum {
	RGB565,
	YUYV_A,
	YUYV_B,
};

static const struct video_format_cap fmts[] = {
	[RGB565] = {.pixelformat = VIDEO_PIX_FMT_RGB565,
		    .width_min  = 1280, .width_max  = 1280, .width_step  = 50,
		    .height_min = 720,  .height_max = 720,  .height_step = 50},
	[YUYV_A] = {.pixelformat = VIDEO_PIX_FMT_YUYV,
		    .width_min  = 100,  .width_max  = 1000, .width_step  = 50,
		    .height_min = 100,  .height_max = 1000, .height_step = 50},
	[YUYV_B] = {.pixelformat = VIDEO_PIX_FMT_YUYV,
		    .width_min  = 1920, .width_max  = 1920, .width_step  = 0,
		    .height_min = 1080, .height_max = 1080, .height_step = 0},
	{0},
};

ZTEST(video_common, test_video_format_caps_index)
{
	struct video_format fmt = {0};
	size_t idx;
	int ret;

	fmt.pixelformat = VIDEO_PIX_FMT_YUYV;

	fmt.width = 100;
	fmt.height = 100;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_ok(ret, "expecting minimum value to match");
	zassert_equal(idx, YUYV_A);

	fmt.width = 1000;
	fmt.height = 1000;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_ok(ret, "expecting maximum value to match");
	zassert_equal(idx, YUYV_A);

	fmt.width = 1920;
	fmt.height = 1080;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_ok(ret, "expecting exact match to work");
	zassert_equal(idx, YUYV_B);

	fmt.width = 1001;
	fmt.height = 1000;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_not_ok(ret, "expecting 1 above maximum width to mismatch");

	fmt.width = 1000;
	fmt.height = 1001;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_not_ok(ret, "expecting 1 above maximum height to mismatch");

	fmt.width = 1280;
	fmt.height = 720;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_not_ok(ret);
	zassert_not_ok(ret, "expecting wrong format to mismatch");

	fmt.pixelformat = VIDEO_PIX_FMT_RGB565;

	fmt.width = 1000;
	fmt.height = 1000;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_not_ok(ret, "expecting wrong format to mismatch");

	fmt.width = 1280;
	fmt.height = 720;
	ret = video_format_caps_index(fmts, &fmt, &idx);
	zassert_ok(ret, "expecting exact match to work");
	zassert_equal(idx, RGB565);
}

ZTEST(video_common, test_video_frmival_nsec)
{
	zassert_equal(
		video_frmival_nsec(&(struct video_frmival){.numerator = 1, .denominator = 15}),
		66666666);

	zassert_equal(
		video_frmival_nsec(&(struct video_frmival){.numerator = 1, .denominator = 30}),
		33333333);

	zassert_equal(
		video_frmival_nsec(&(struct video_frmival){.numerator = 5, .denominator = 1}),
		5000000000);

	zassert_equal(
		video_frmival_nsec(&(struct video_frmival){.numerator = 1, .denominator = 1750000}),
		571);
}

ZTEST(video_common, test_video_closest_frmival_stepwise)
{
	struct video_frmival_stepwise stepwise;
	uint64_t desired;
	uint64_t expected;
	uint64_t match;
	int ret;

	stepwise.min = NSEC_PER_SEC / 30;
	stepwise.max = 30 * (NSEC_PER_SEC / 30);
	stepwise.step = NSEC_PER_SEC / 30;

	desired = NSEC_PER_SEC;
	ret = video_closest_frmival_stepwise(&stepwise, desired, &match);
	zassert_ok(ret, "expecting video_closest_frmival_stepwise to work");
	zassert_equal(match, stepwise.max, "1 / 1");

	desired = 3 * (NSEC_PER_SEC / 30);
	ret = video_closest_frmival_stepwise(&stepwise, desired, &match);
	zassert_ok(ret, "expecting video_closest_frmival_stepwise to work");
	zassert_equal(match, desired, "3 / 30");

	desired = (uint64_t)NSEC_PER_SEC * 7 / 80;
	expected = 3 * (NSEC_PER_SEC / 30);
	ret = video_closest_frmival_stepwise(&stepwise, desired, &match);
	zassert_ok(ret, "expecting video_closest_frmival_stepwise to work");
	zassert_equal(match, expected, "7 / 80");

	desired = NSEC_PER_SEC / 120;
	ret = video_closest_frmival_stepwise(&stepwise, desired, &match);
	zassert_ok(ret, "expecting video_closest_frmival_stepwise to work");
	zassert_equal(match, stepwise.min, "1 / 120");

	desired = 100ULL * NSEC_PER_SEC;
	ret = video_closest_frmival_stepwise(&stepwise, desired, &match);
	zassert_ok(ret, "expecting video_closest_frmival_stepwise to work");
	zassert_equal(match, stepwise.max, "100 / 1");

	/* Fine 1ms step test with large max */
	stepwise.min = NSEC_PER_SEC / 60;
	stepwise.max = UINT64_MAX;
	stepwise.step = NSEC_PER_MSEC;

	desired = 16667ULL * NSEC_PER_USEC;
	ret = video_closest_frmival_stepwise(&stepwise, desired, &match);
	zassert_ok(ret, "expecting video_closest_frmival_stepwise to work");
	zassert_equal(match, stepwise.min, "16667 / 1000000");

	desired = 33333ULL * NSEC_PER_USEC;
	expected = stepwise.min +
		   DIV_ROUND_CLOSEST(desired - stepwise.min, stepwise.step) * stepwise.step;
	ret = video_closest_frmival_stepwise(&stepwise, desired, &match);
	zassert_ok(ret, "expecting video_closest_frmival_stepwise to work");
	zassert_equal(match, expected, "33333 / 1000000");
}

ZTEST(video_common, test_video_buffer_release_null)
{
	int ret;

	ret = video_buffer_release(NULL);
	zassert_equal(ret, -EINVAL, "expecting -EINVAL when releasing a NULL buffer");
}

ZTEST(video_common, test_video_buffer_alloc_release)
{
	struct video_buffer *vbuf;
	int ret;

	vbuf = video_buffer_alloc(64, K_NO_WAIT);
	zassert_not_null(vbuf, "expecting buffer allocation to succeed");

	ret = video_buffer_release(vbuf);
	zassert_ok(ret, "expecting buffer release to succeed");

	ret = video_buffer_release(vbuf);
	zassert_equal(ret, -EINVAL, "expecting -EINVAL when releasing a buffer twice");
}

ZTEST(video_common, test_video_buffer_release_bad_index)
{
	struct video_buffer vbuf = {.index = CONFIG_VIDEO_BUFFER_POOL_NUM_MAX};
	int ret;

	ret = video_buffer_release(&vbuf);
	zassert_equal(ret, -EINVAL, "expecting -EINVAL for an out-of-range buffer index");
}

ZTEST_SUITE(video_common, NULL, NULL, NULL, NULL, NULL);
