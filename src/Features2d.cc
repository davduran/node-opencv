#include "Features2d.h"
#include "Matrix.h"
#include <nan.h>
#include <stdio.h>

#if CV_MAJOR_VERSION >= 2 && CV_MINOR_VERSION >=4

void
Features::Init(Handle<Object> target) {
  NanScope();

  NODE_SET_METHOD(target, "ImageSimilarity", Similarity);
};

class AsyncDetectSimilarity : public NanAsyncWorker {
 public:
  AsyncDetectSimilarity(NanCallback *callback, cv::Mat image1, cv::Mat image2) : NanAsyncWorker(callback), image1(image1), image2(image2), dissimilarity(0) {}
  ~AsyncDetectSimilarity() {}

  void Execute () {

      cv::Ptr<cv::FeatureDetector> detector = cv::FeatureDetector::create("ORB");
      cv::Ptr<cv::DescriptorExtractor> extractor = cv::DescriptorExtractor::create("ORB");
      cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create("BruteForce-Hamming");

      std::vector<cv::DMatch> matches;

      cv::Mat descriptors1 = cv::Mat();
      cv::Mat descriptors2 = cv::Mat();

      std::vector<cv::KeyPoint> keypoints1;
      std::vector<cv::KeyPoint> keypoints2;

      detector->detect(image1, keypoints1);
      detector->detect(image2, keypoints2);

      extractor->compute(image1, keypoints1, descriptors1);
      extractor->compute(image2, keypoints2, descriptors2);

      matcher->match(descriptors1, descriptors2, matches);

      double max_dist = 0;
      double min_dist = 100;

      //-- Quick calculation of max and min distances between keypoints
      for (int i = 0; i < descriptors1.rows; i++) {
        double dist = matches[i].distance;
        if( dist < min_dist ) min_dist = dist;
        if( dist > max_dist ) max_dist = dist;
      }

      //-- Draw only "good" matches (i.e. whose distance is less than 2*min_dist,
      //-- or a small arbitary value ( 0.02 ) in the event that min_dist is very
      //-- small)
      //-- PS.- radiusMatch can also be used here.
      std::vector<cv::DMatch> good_matches;
      double good_matches_sum = 0.0;

      for (int i = 0; i < descriptors1.rows; i++ ) {
        double distance = matches[i].distance;
        if (distance <= std::max(2*min_dist, 0.02)) {
          good_matches.push_back(matches[i]);
          good_matches_sum += distance;
        }
      }

      dissimilarity = (double)good_matches_sum / (double)good_matches.size();

  }

  void HandleOKCallback () {
    NanScope();

    Handle<Value> argv[2];

    argv[0] = NanNull();
    argv[1] = NanNew<Number>(dissimilarity);

    callback->Call(2, argv);

  }

  private:
    cv::Mat image1;
    cv::Mat image2;
    double dissimilarity;

};

NAN_METHOD(Features::Similarity) {
  NanScope();

  REQ_FUN_ARG(2, cb);

  cv::Mat image1 = ObjectWrap::Unwrap<Matrix>(args[0]->ToObject())->mat;
  cv::Mat image2 = ObjectWrap::Unwrap<Matrix>(args[1]->ToObject())->mat;

  NanCallback *callback = new NanCallback(cb.As<Function>());

  NanAsyncQueueWorker( new AsyncDetectSimilarity(callback, image1, image2) );
  NanReturnUndefined();

};

#endif
