#ifndef IMAGE_TEST_H__ 
#define  IMAGE_TEST_H__

/* BUILD_IMAGE_TEST_PROGRAM is controlled by top level CMakelists.txt */
#ifdef BUILD_IMAGE_TEST_PROGRAM
void image_test_cb(struct badge_app *app);
#endif

#endif
