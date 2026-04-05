#include "ImageStateManager.h"

void ImageStateManager::setImageA(const cv::Mat& img) { m_imgA = img.clone(); }
void ImageStateManager::setImageB(const cv::Mat& img) { m_imgB = img.clone(); }
void ImageStateManager::setOutput(const cv::Mat& img) { m_output = img.clone(); }

cv::Mat ImageStateManager::getImageA() const { return m_imgA.clone(); }
cv::Mat ImageStateManager::getImageB() const { return m_imgB.clone(); }
cv::Mat ImageStateManager::getOutput()  const { return m_output.clone(); }

bool ImageStateManager::hasImageA() const { return !m_imgA.empty(); }
bool ImageStateManager::hasImageB() const { return !m_imgB.empty(); }

void ImageStateManager::clearOutput() { m_output.release(); }
void ImageStateManager::clearAll()    { m_imgA.release(); m_imgB.release(); m_output.release(); }