/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: Rajib Bhattacharjea<raj.b@gatech.edu>
//

// Georgia Tech Network Simulator - Round Trip Time Estimation Class
// George F. Riley.  Georgia Tech, Spring 2002


#ifndef RTT_ESTIMATOR_H
#define RTT_ESTIMATOR_H

#include <deque>
#include "ns3/sequence-number.h"
#include "ns3/nstime.h"
#include "ns3/object.h"

namespace ns3 {

/**
 * \ingroup tcp
 *
 * \brief Helper class to store RTT measurements
 */
class RttHistory {
public:
  /**
   * \brief Constructor - builds an RttHistory with the given parameters
   * \param s First sequence number in packet sent
   * \param c Number of bytes sent
   * \param t Time this one was sent
   */
  RttHistory (SequenceNumber32 s, uint32_t c, Time t, uint64_t marked, uint64_t unmarked);
  /**
   * \brief Copy constructor
   * \param h the object to copy
   */
  RttHistory (const RttHistory& h); // Copy constructor
public:
  SequenceNumber32  seq;  //!< First sequence number in packet sent
  uint32_t        count;  //!< Number of bytes sent
  Time            time;   //!< Time this one was sent
  uint64_t        nonMarked; // Number of unmarked packets (needed for DCTCP)
  uint64_t        marked;   // Number of marked packets (needed for DCTCP)
  bool            retx;   //!< True if this has been retransmitted
};

/// Container for RttHistory objects
typedef std::deque<RttHistory> RttHistory_t;

/**
 * \ingroup tcp
 *
 * \brief Base class for all RTT Estimators
 */
class RttEstimator : public Object {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  RttEstimator();
  /**
   * \brief Copy constructor
   * \param r the object to copy
   */
  RttEstimator (const RttEstimator& r);

  virtual ~RttEstimator();

  virtual TypeId GetInstanceTypeId (void) const;

  /**
   * \brief Note that a particular sequence has been sent
   * \param seq the packet sequence number.
   * \param size the packet size.
   */
  virtual void SentSeq (SequenceNumber32 seq, uint32_t size);

  /**
   * \brief Note that a particular ack sequence has been received
   * \param ackSeq the ack sequence number.
   * \return The measured RTT for this ack.
   */
  virtual Time AckSeq (SequenceNumber32 ackSeq, bool markedFlag = false);

  /**
   * \brief Clear all history entries
   */
  virtual void ClearSent ();

  /**
   * \brief Add a new measurement to the estimator. Pure virtual function.
   * \param t the new RTT measure.
   */
  virtual void  Measurement (Time t) = 0;

  /**
   * \brief Returns the estimated RTO. Pure virtual function.
   * \return the estimated RTO.
   */
  virtual Time RetransmitTimeout () = 0;

  /**
   * \brief Copy object
   * \returns a copy of itself
   */
  virtual Ptr<RttEstimator> Copy () const = 0;

  /**
   * \brief Increase the estimation multiplier up to MaxMultiplier.
   */
  virtual void IncreaseMultiplier ();

  /**
   * \brief Resets the estimation multiplier to 1.
   */
  virtual void ResetMultiplier ();

  /**
   * \brief Resets the estimation to its initial state.
   */
  virtual void Reset ();

  /**
   * \brief Sets the Minimum RTO.
   * \param minRto The minimum RTO returned by the estimator.
   */
  void SetMinRto (Time minRto);

  /**
   * \brief Get the Minimum RTO.
   * \return The minimum RTO returned by the estimator.
   */
  Time GetMinRto (void) const;

  /**
   * \brief Sets the current RTT estimate (forcefully).
   * \param estimate The current RTT estimate.
   */
  void SetCurrentEstimate (Time estimate);

  /**
   * \brief gets the current RTT estimate.
   * \return The current RTT estimate.
   */
  Time GetCurrentEstimate (void) const;
    /**
   * \brief gets the current Alpha value.
   * \return The current Alpha value.
   */
  double GetAlpha (void) const;

  /**
   * \brief gets the current DCTCP weight value.
   * \return The current parameter g value.
   */
  double GetG (void) const;

  /**
   * \brief sets the current DCTCP weight value.
   * \return The current parameter g value.
   */
  void SetG (double g);
  
  void Init(SequenceNumber32 s) {m_next = s;} // It will be used by mptcp module
private:
  SequenceNumber32 m_next;    //!< Next expected sequence to be sent
  RttHistory_t m_history;     //!< List of sent packet
  uint16_t m_maxMultiplier;   //!< Maximum RTO Multiplier
  Time m_initialEstimatedRtt; //!< Initial RTT estimation

protected:
  Time         m_currentEstimatedRtt;     //!< Current estimate
  Time         m_minRto;                  //!< minimum value of the timeout
  uint32_t     m_nSamples;                //!< Number of samples
  uint16_t     m_multiplier;              //!< RTO Multiplier
public:
  // Parameters needed for DCTCP
  double       m_g;
  uint64_t     m_marked;
  uint64_t     m_nonMarked;
  double       m_alpha;
  double m_delta_marked;
  double m_delta_unmarked;
  double m_fracMarkPkt;
};

/**
 * \ingroup tcp
 *
 * \brief The "Mean--Deviation" RTT estimator, as discussed by Van Jacobson
 *
 * This class implements the "Mean--Deviation" RTT estimator, as discussed
 * by Van Jacobson and Michael J. Karels, in
 * "Congestion Avoidance and Control", SIGCOMM 88, Appendix A
 *
 */
class RttMeanDeviation : public RttEstimator {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  RttMeanDeviation ();

  /**
   * \brief Copy constructor
   * \param r the object to copy
   */
  RttMeanDeviation (const RttMeanDeviation& r);

  virtual TypeId GetInstanceTypeId (void) const;

  /**
   * \brief Add a new measurement to the estimator.
   * \param measure the new RTT measure.
   */
  void Measurement (Time measure);

  /**
   * \brief Returns the estimated RTO.
   * \return the estimated RTO.
   */
  Time RetransmitTimeout ();

  Ptr<RttEstimator> Copy () const;

  /**
   * \brief Resets the estimator.
   */
  void Reset ();

  /**
   * \brief Sets the estimator Gain.
   * \param g the gain, where 0 < g < 1.
   */
  void Gain (double g);

private:
  double       m_gain;       //!< Filter gain
  Time         m_variance;   //!< Current variance
};
} // namespace ns3

#endif /* RTT_ESTIMATOR_H */