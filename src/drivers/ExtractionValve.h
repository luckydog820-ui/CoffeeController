#ifndef COFFEE_CONTROLLER_EXTRACTION_VALVE_H_
#define COFFEE_CONTROLLER_EXTRACTION_VALVE_H_

class ExtractionValve {
 public:
  void begin();
  void setOpen(bool open);
  void emergencyClose();
  bool isOpen() const { return open_; }

 private:
  bool open_ = false;
};

#endif  // COFFEE_CONTROLLER_EXTRACTION_VALVE_H_
